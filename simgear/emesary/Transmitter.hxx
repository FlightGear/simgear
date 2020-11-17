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
#include <list>
#include <set>
#include <vector>
#include <atomic>
#include <mutex>

namespace simgear
{
    namespace Emesary
    {
        // Implementation of a ITransmitter
        class Transmitter : public ITransmitter
        {
        protected:
            typedef std::list<IReceiver *> RecipientList;
            RecipientList recipient_list;
            RecipientList deleted_recipients;
            int CurrentRecipientIndex = 0;
            std::mutex _lock;
            std::atomic<int> receiveDepth;
            std::atomic<int> sentMessageCount;

        public:
            Transmitter() : receiveDepth(0), sentMessageCount(0)
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
            virtual void Register(IReceiver& r)
            {
                std::lock_guard<std::mutex> scopeLock(_lock);
                recipient_list.push_back(&r);
                r.OnRegisteredAtTransmitter(this);
                if (std::find(deleted_recipients.begin(), deleted_recipients.end(), &r) != deleted_recipients.end())
                    deleted_recipients.remove(&r);
            }

            //  Removes an object from receving message from this transmitter
            virtual void DeRegister(IReceiver& R)
            {
                std::lock_guard<std::mutex> scopeLock(_lock);
                if (recipient_list.size())
                {
                    if (std::find(recipient_list.begin(), recipient_list.end(), &R) != recipient_list.end())
                    {
                        recipient_list.remove(&R);
                        R.OnDeRegisteredAtTransmitter(this);
                        if (std::find(deleted_recipients.begin(), deleted_recipients.end(), &R) == deleted_recipients.end())
                            deleted_recipients.push_back(&R);
                    }
                }
            }

            // Notify all registered recipients. Stop when receipt status of abort or finished are received.
            // The receipt status from this method will be
            //  - OK > message handled
            //  - Fail > message not handled. A status of Abort from a recipient will result in our status
            //           being fail as Abort means that the message was not and cannot be handled, and
            //           allows for usages such as access controls.
            virtual ReceiptStatus NotifyAll(INotification& M)
            {
                ReceiptStatus return_status = ReceiptStatusNotProcessed;

                sentMessageCount++;

                std::vector<IReceiver*> temp;
                {
                    std::lock_guard<std::mutex> scopeLock(_lock);
                    if (receiveDepth == 0)
                        deleted_recipients.clear();
                    receiveDepth++;

                    int idx = 0;
                    for (RecipientList::iterator i = recipient_list.begin(); i != recipient_list.end(); i++)
                    {
                        temp.push_back(*i);
                    }
                }
                int tempSize = temp.size();
                for (int index = 0; index < tempSize; index++)
                {
                    IReceiver* R = temp[index];
                    {
                        std::lock_guard<std::mutex> scopeLock(_lock);
                        if (deleted_recipients.size())
                        {
                            if (std::find(deleted_recipients.begin(), deleted_recipients.end(), R) != deleted_recipients.end())
                            {
                                continue;
                            }
                        }
                    }
                    if (R)
                    {
                        ReceiptStatus rstat = R->Receive(M);
                        switch (rstat)
                        {
                        case ReceiptStatusFail:
                            return_status = ReceiptStatusFail;
                            break;

                        case ReceiptStatusPending:
                            return_status = ReceiptStatusPending;
                            break;

                        case ReceiptStatusPendingFinished:
                            return rstat;

                        case ReceiptStatusNotProcessed:
                            break;

                        case ReceiptStatusOK:
                            if (return_status == ReceiptStatusNotProcessed)
                                return_status = rstat;
                            break;

                        case ReceiptStatusAbort:
                            return ReceiptStatusAbort;

                        case ReceiptStatusFinished:
                            return ReceiptStatusOK;
                        }
                    }

                }

                receiveDepth--;
                return return_status;
            }

            // number of currently registered recipients
            virtual int Count()
            {
                std::lock_guard<std::mutex> scopeLock(_lock);
                return recipient_list.size();
            }

            // number of sent messages.
            int SentMessageCount()
            {
                return sentMessageCount;
            }

            // ascertain if a receipt status can be interpreted as failure.
            static bool Failed(ReceiptStatus receiptStatus)
            {
                //
                // failed is either Fail or Abort.
                // NotProcessed isn't a failure because it hasn't been processed.
                return receiptStatus == ReceiptStatusFail
                    || receiptStatus == ReceiptStatusAbort;
            }
        };
    }
}
#endif
