#ifndef TRANSMITTER_hxx
#define TRANSMITTER_hxx
/*---------------------------------------------------------------------------
*
*  Title                : Emesary - Transmitter base class
*
*  File Type            : Implementation File
*
*  Description          : Defines the receipt status that can be returned from
*                       : a receive method.
*
*  References           : http://www.chateau-logic.com/content/class-based-inter-object-communication
*
*  Author               : Richard Harrison (richard@zaretto.com)
*
*  Creation Date        : 18 March 2002, rewrite 2017, simgear version 2019
*
*  Version              : $Header: $
*
*  Copyright (C)2019 Richard Harrison            Licenced under GPL2 or later.
*
*---------------------------------------------------------------------------*/

#include <algorithm>
#include <string>
#include <vector>
#include <set>
#include <vector>
#include <atomic>
#include <mutex>
#include <cstddef>
#include "ITransmitter.hxx"

namespace simgear
{
	namespace Emesary
	{
		// Implementation of a ITransmitter
		class Transmitter : public ITransmitter
		{
		protected:
		typedef std::vector<IReceiverPtr> RecipientList;
			RecipientList recipient_list;
			RecipientList new_recipient_list;
			RecipientList deleted_recipient_list;

			std::mutex _lock;
			std::atomic<size_t> receiveDepth;
			std::atomic<size_t> sentMessageCount;
			std::atomic<size_t> recipientCount;
			std::atomic<size_t> pendingDeletions;
			std::atomic<size_t> pendingAdditions;

		public:
			Transmitter() :
				receiveDepth(0),
				sentMessageCount(0),
				recipientCount(0),
				pendingDeletions(0),
				pendingAdditions(0)
			{
			}

			virtual ~Transmitter()
			{
			}

			// Registers an object to receive messsages from this transmitter.
			// This object is added to the top of the list of objects to be notified. This is deliberate as
			// the sequence of registration and message receipt can influence the way messages are processing
			// when ReceiptStatus of Abort or Finished are encountered. So it was a deliberate decision that the
			// most recently registered recipients should process the messages/events first.
			virtual void Register(IReceiverPtr r)
			{
				std::lock_guard<std::mutex> scopeLock(_lock);

				RecipientList::iterator deleted_location = std::find(deleted_recipient_list.begin(), deleted_recipient_list.end(), r);
				if (deleted_location != deleted_recipient_list.end())
					deleted_recipient_list.erase(deleted_location);

				RecipientList::iterator location = std::find(recipient_list.begin(), recipient_list.end(), r);
				if (location == recipient_list.end())
				{
					RecipientList::iterator location = std::find(new_recipient_list.begin(), new_recipient_list.end(), r);
					if (location == new_recipient_list.end()) {
						new_recipient_list.insert(new_recipient_list.begin(), r);
						pendingAdditions++;
					}
				}
			}

			///  Removes an object from receving message from this transmitter. 
			/// NOTES: OnDeRegisteredAtTransmitter will be called as a result of this method
			///        If recipient is in the list of new recipients it will be removed from that list
			virtual void DeRegister(IReceiverPtr r)
			{
				std::lock_guard<std::mutex> scopeLock(_lock);

				if (new_recipient_list.size())
				{
					RecipientList::iterator location = std::find(new_recipient_list.begin(), new_recipient_list.end(), r);

					if (location != new_recipient_list.end())
						new_recipient_list.erase(location);
				}
				deleted_recipient_list.push_back(r);
				r->OnDeRegisteredAtTransmitter(this);
				pendingDeletions++;
			}

			// this will purge the recipients that are marked as deleted
			// it will only do this when the receive depth is zero - i.e. this 
			// notification is being sent out from outside a recipient notify.(because it is 
			// fine for a recipient to retransmit another notification as a result of receiving a notification)
			//
			// also we can quickly check to see if we have any pending deletions before doing anything.
			void AddRemoveIFAppropriate()
			{
				std::lock_guard<std::mutex> scopeLock(_lock);

				/// handle pending deletions first.
				if (pendingDeletions > 0) {

					///
					/// remove deleted recipients from the main list.
					std::for_each(deleted_recipient_list.begin(), deleted_recipient_list.end(),
						[this](IReceiverPtr r) {

						RecipientList::iterator location = std::find(recipient_list.begin(), recipient_list.end(), r);
						if (location != recipient_list.end()) {
							recipient_list.erase(location);
						}
					});
					recipientCount -= pendingDeletions;
					deleted_recipient_list.erase(deleted_recipient_list.begin(), deleted_recipient_list.end());
					pendingDeletions = 0; // can do this because we are guarded
				}

				if (pendingAdditions) {
					/// firstly remove items from the new list that are already in the list
					std::for_each(recipient_list.begin(), recipient_list.end(),
						[this](IReceiverPtr r) {

						RecipientList::iterator location = std::find(new_recipient_list.begin(), new_recipient_list.end(), r);
						if (location != new_recipient_list.end()) {
							new_recipient_list.erase(location);
							pendingAdditions--;
						}
					});


					std::for_each(new_recipient_list.begin(), new_recipient_list.end(),
						[this](IReceiverPtr r) {
						r->OnRegisteredAtTransmitter(this);
					});
					recipient_list.insert(recipient_list.begin(),
						std::make_move_iterator(new_recipient_list.begin()),
						std::make_move_iterator(new_recipient_list.end()));

					new_recipient_list.erase(new_recipient_list.begin(), new_recipient_list.end());
					recipientCount += pendingAdditions;
					pendingAdditions = 0;

				}
			}
			// Notify all registered recipients. Stop when receipt status of abort or finished are received.
			// The receipt status from this method will be
			//  - OK > message handled
			//  - Fail > message not handled. A status of Abort from a recipient will result in our status
			//           being fail as Abort means that the message was not and cannot be handled, and
			//           allows for usages such as access controls.
			virtual ReceiptStatus NotifyAll(INotificationPtr M)
			{
				ReceiptStatus return_status = ReceiptStatus::NotProcessed;

				auto v = receiveDepth.fetch_add(1, std::memory_order_relaxed);
				if (v == 0)
					AddRemoveIFAppropriate();

				sentMessageCount++;

				bool finished = false;

				size_t idx = 0;
				do {

					if (idx < recipient_list.size()) {
						IReceiverPtr R = recipient_list[idx++];

						if (R != nullptr)
						{
							Emesary::ReceiptStatus rstat = R->Receive(M);
							switch (rstat)
							{
							case ReceiptStatus::Fail:
								return_status = ReceiptStatus::Fail;
								break;

							case ReceiptStatus::Pending:
								return_status = ReceiptStatus::Pending;
								break;

							case ReceiptStatus::PendingFinished:
								return_status = rstat;
								finished = true;
								break;

							case ReceiptStatus::NotProcessed:
								break;

							case ReceiptStatus::OK:
								if (return_status == ReceiptStatus::NotProcessed)
									return_status = rstat;
								break;

							case ReceiptStatus::Abort:
								finished = true;
								return_status = ReceiptStatus::Abort;
								break;

							case ReceiptStatus::Finished:
								finished = true;
								return_status = ReceiptStatus::OK;;
								break;
							}
						}

					}
					else
						break;
				} while (!finished);
				receiveDepth--;
				return return_status;
			}

			// number of sent messages.
			size_t SentMessageCount() const
			{
				return sentMessageCount;
			}

			// number of currently registered recipients
			// better to avoid using the size members on the list directly as
			// using the atomics is lock free and threadsafe.
			virtual size_t Count() const
			{
				return (recipientCount + pendingAdditions) - pendingDeletions;
			}
			// ascertain if a receipt status can be interpreted as failure.
			static bool Failed(ReceiptStatus receiptStatus)
			{
				//
				// failed is either Fail or Abort.
				// NotProcessed isn't a failure because it hasn't been processed.
				return receiptStatus == ReceiptStatus::Fail
					|| receiptStatus == ReceiptStatus::Abort;
			}
		};
	}
}
#endif
