using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading;

namespace VVVV.Nodes.Bluefish
{
	public class WorkerThread : IDisposable
	{
		public delegate void WorkItemDelegate();
		public static WorkerThread Singleton = new WorkerThread();

		Thread FThread;
		bool FRunning;
		Object FLock = new Object();
		Queue<WorkItemDelegate> FWorkQueue = new Queue<WorkItemDelegate>();
		Exception FException;
        ManualResetEvent FNewWorkEvent = new ManualResetEvent(false);
        ManualResetEvent FEmptyQueueEvent = new ManualResetEvent(true);

		public WorkerThread()
		{
			FRunning = true;
			FThread = new Thread(Loop);
			FThread.SetApartmentState(ApartmentState.MTA);
			FThread.Name = "Bluefish Worker";
			FThread.Start();
		}

		void Loop()
		{
			while (FRunning)
			{
                FNewWorkEvent.WaitOne();
                WorkItemDelegate nextWork;
                do
                {
                    lock (FLock)
                    {
                        if (FWorkQueue.Count > 0)
                        {
                            nextWork = FWorkQueue.Dequeue();
                        }
                        else
                        {
                            nextWork = null;
                            FEmptyQueueEvent.Set();
                            FNewWorkEvent.Reset();
                        }
                    }
                    if (nextWork!=null)
                    {
                        try
                        {
                            nextWork();
                            this.FException = null;
                        }
                        catch (Exception e)
                        {
                            this.FException = e;
                        }
                    }
                } while (nextWork != null);
			}
		}

		public void Dispose()
		{
			FRunning = false;
			FThread.Join();
		}

		public void PerformBlocking(WorkItemDelegate item)
		{
			if (Thread.CurrentThread == this.FThread)
			{
				//we're already inside the worker thread
				item();
			}
			else
			{
				Perform(item);
				BlockUntilEmpty();
				if (this.FException != null)
				{
					var e = this.FException;
					this.FException = null;
					throw (e);
				}
			}
		}

		public void Perform(WorkItemDelegate item)
		{
			lock (FLock)
			{
				FWorkQueue.Enqueue(item);
                FNewWorkEvent.Set();
                FEmptyQueueEvent.Reset();
			}
		}

		public void PerformUnique(WorkItemDelegate item)
		{
			lock (FLock)
			{
                if (!FWorkQueue.Contains(item))
                {
                    FWorkQueue.Enqueue(item);
                    FNewWorkEvent.Set();
                    FEmptyQueueEvent.Reset();
                }
			}
		}

		public void BlockUntilEmpty()
		{
            /*lock (FLock)
            {
                if (FWorkQueue.Count == 0)
                    return;
            }*/
            FEmptyQueueEvent.WaitOne();
		}

        public int QueueSize{
            get{
                lock (FLock)
			    {
                    return FWorkQueue.Count;
                }
            }
        }
	}
}
