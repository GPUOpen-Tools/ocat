//
// Copyright(c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

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
