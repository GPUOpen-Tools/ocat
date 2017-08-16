using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Frontend
{
    class DelayTimer
    {
        // Called with remaining time in MS.
        private Action<int> onTickFunc;
        private Action onEndFunc;
        bool running;

        // onTickFunc receives the remaining time in ms. 
        public DelayTimer(Action<int> onTickFunc, Action onEndFunc)
        {
            this.onTickFunc = onTickFunc;
            this.onEndFunc = onEndFunc;
            running = false;
        }

        public async void Start(int timeInMS, int intervalInMS)
        {
            running = true;

            for (int i = 0; i <= timeInMS; i += intervalInMS)
            {
                await Task.Delay(intervalInMS);
                if (running)
                {
                    onTickFunc(timeInMS - i);
                }
                else
                {
                    running = false;
                    return;
                }
            }
            running = false;
            onEndFunc();
        }

        public bool IsRunning()
        {
            return running;
        }

        public void Stop()
        {
            running = false;
        }
    }
}
