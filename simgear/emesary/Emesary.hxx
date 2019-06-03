#pragma once
/*---------------------------------------------------------------------------
*
*	Title                : Emesary - class based inter-object communication
*
*	File Type            : Implementation File
*
*	Description          : Provides generic inter-object communication. For an object to receive a message it
*	                     : must first register with a Transmitter, such as GlobalTransmitter, and implement the
*	                     : IReceiver interface. That's it.
*	                     : To send a message use a Transmitter with an object. That's all there is to it.
*
*  References           : http://www.chateau-logic.com/content/class-based-inter-object-communication
*
*	Author               : Richard Harrison (richard@zaretto.com)
*
*	Creation Date        : 18 March 2002, rewrite 2017
*
*	Version              : $Header: $
*
*  Copyright © 2002 - 2017 Richard Harrison           All Rights Reserved.
*
*---------------------------------------------------------------------------*/
#include <typeinfo>

#include <string>
#include <list>
#include <set>
#include <vector>
#include <atomic>
#include <simgear/threads/SGThread.hxx>


namespace simgear
{
    namespace Emesary
    {
        enum ReceiptStatus
        {
            /// <summary>
            /// Processing completed successfully
            /// </summary>
            ReceiptStatusOK = 0,

            /// <summary>
            /// Individual item failure
            /// </summary>
            ReceiptStatusFail = 1,

            /// <summary>
            /// Fatal error; stop processing any further recipieints of this message. Implicitly fail
            /// </summary>
            ReceiptStatusAbort = 2,

            /// <summary>
            /// Definitive completion - do not send message to any further recipieints
            /// </summary>
            ReceiptStatusFinished = 3,

            /// <summary>
            /// Return value when method doesn't process a message.
            /// </summary>
            ReceiptStatusNotProcessed = 4,

            /// <summary>
            /// Message has been sent but the return status cannot be determined as it has not been processed by the recipient. 
            /// </summary>
            /// <notes>
            /// For example a queue or outgoing bridge
            /// </notes>
            ReceiptStatusPending = 5,

            /// <summary>
            /// Message has been definitively handled but the return value cannot be determined. The message will not be sent any further
            /// </summary>
            /// <notes>
            /// For example a point to point forwarding bridge
            /// </notes>
            ReceiptStatusPendingFinished = 6,
        };

        /// <summary>
        /// Interface (base class) for all notifications. The value is an opaque pointer that may be used to store anything, although
        /// often it is more convenient to 
        /// </summary>
        class INotification
        {
        public:
            virtual const char *GetType() = 0;
        };
        /// <summary>
        /// Interface (base class) for a recipeint.
        /// </summary>
        class IReceiver
        {
        public:
            /// <summary>
            /// Receive notifiction - must be implemented
            /// </summary>
            virtual ReceiptStatus Receive(INotification& message) = 0;

            /// <summary>
            /// Called when registered at a transmitter
            /// </summary>
            virtual void OnRegisteredAtTransmitter(class Transmitter *p)
            {
            }
            /// <summary>
            /// Called when de-registered at a transmitter
            /// </summary>
            virtual void OnDeRegisteredAtTransmitter(class Transmitter *p)
            {
            }
        };

        /// <summary>
        ///  Interface (base clasee) for a transmitter.
        /// Transmits Message derived objects. Each instance of this class provides a
        ///  databus to which any number of receivers can attach to.
        /// </summary>
        class ITransmitter
        {
        public:
            /*
            *  Registers a recipient to receive message from this transmitter
            */
            virtual void Register(IReceiver& R) = 0;
            /*
            *  Removes a recipient from from this transmitter
            */
            virtual void DeRegister(IReceiver& R) = 0;

            /*
            * Notify all registered recipients. Stop when receipt status of abort or finished are received.
            * The receipt status from this method will be
            *  - OK > message handled
            *  - Fail > message not handled. A status of Abort from a recipient will result in our status
            *           being fail as Abort means that the message was not and cannot be handled, and
            *           allows for usages such as access controls.
            */
            virtual ReceiptStatus NotifyAll(INotification& M) = 0;
            /// <summary>
            /// number of recipients
            /// </summary>
            virtual int Count() = 0;
        };


        /**
        * Description: Transmits Message derived objects. Each instance of this class provides a
        * databus to which any number of receivers can attach to.
        *
        * Messages may be inherited and customised between individual systems.
        */
        class Transmitter : public ITransmitter
        {
        protected:
            typedef std::list<IReceiver *> RecipientList;
            RecipientList recipient_list;
            RecipientList deleted_recipients;
            int CurrentRecipientIndex = 0;
            SGMutex _lock;
            std::atomic<int> receiveDepth;
            std::atomic<int> sentMessageCount;

            void UnlockList()
            {
                _lock.unlock();
            }
            void LockList()
            {
                _lock.lock();
            }
        public:
            Transmitter() : receiveDepth(0), sentMessageCount(0)
            {
            }
            virtual ~Transmitter()
            {
            }
            /**
            * Registers an object to receive messsages from this transmitter.
            * This object is added to the top of the list of objects to be notified. This is deliberate as
            * the sequence of registration and message receipt can influence the way messages are processing
            * when ReceiptStatus of Abort or Finished are encountered. So it was a deliberate decision that the
            * most recently registered recipients should process the messages/events first.
            */
            virtual void Register(IReceiver& r)
            {
                LockList();
                recipient_list.push_back(&r);
                r.OnRegisteredAtTransmitter(this);
                if (std::find(deleted_recipients.begin(), deleted_recipients.end(), &r) != deleted_recipients.end())
                    deleted_recipients.remove(&r);

                UnlockList();
            }

            /*
            *  Removes an object from receving message from this transmitter
            */
            virtual void DeRegister(IReceiver& R)
            {
                LockList();
                //printf("Remove %x\n", &R);
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
                UnlockList();
            }

            /*
            * Notify all registered recipients. Stop when receipt status of abort or finished are received.
            * The receipt status from this method will be
            *  - OK > message handled
            *  - Fail > message not handled. A status of Abort from a recipient will result in our status
            *           being fail as Abort means that the message was not and cannot be handled, and
            *           allows for usages such as access controls.
            * NOTE: When I first designed Emesary I always intended to have message routing and the ability
            *       for each recipient to specify an area of interest to allow performance improvements
            *       however this has not yet been implemented - but the concept is still there and
            *       could be implemented by extending the IReceiver interface to allow for this.
            */
            virtual ReceiptStatus NotifyAll(INotification& M)
            {
                ReceiptStatus return_status = ReceiptStatusNotProcessed;
                //printf("Begin receive %d : %x\n", (int)receiveDepth, M);
                //fflush(stdout);
                sentMessageCount++;
                try
                {
                    LockList();
                    if (receiveDepth == 0)
                        deleted_recipients.clear();
                    receiveDepth++;
                    std::vector<IReceiver*> temp(recipient_list.size());
                    int idx = 0;
                    for (RecipientList::iterator i = recipient_list.begin(); i != recipient_list.end(); i++)
                    {
                        temp[idx++] = *i;
                    }
                    UnlockList();
                    int tempSize = temp.size();
                    for (int index = 0; index < tempSize; index++)
                    {
                        IReceiver* R = temp[index];
                        LockList();
                        if (deleted_recipients.size())
                        {
                            if (std::find(deleted_recipients.begin(), deleted_recipients.end(), R) != deleted_recipients.end())
                            {
                                UnlockList();
                                continue;
                            }
                        }
                        UnlockList();
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
                }
                catch (...)
                {
                    throw;
                    // return_status = ReceiptStatusAbort;
                }
                receiveDepth--;
                //printf("End receive %d : %x\n", (int) receiveDepth, M);
                return return_status;
            }
            virtual int Count()
            {
                LockList();
                return recipient_list.size();
                UnlockList();
            }
            int SentMessageCount()
            {
                return sentMessageCount;
            }
            static bool Failed(ReceiptStatus receiptStatus)
            {
                //
                // failed is either Fail or Abort.
                // NotProcessed isn't a failure because it hasn't been processed.
                return receiptStatus == ReceiptStatusFail
                    || receiptStatus == ReceiptStatusAbort;
            }
        };
        Transmitter GlobalTransmitter;
    }
}