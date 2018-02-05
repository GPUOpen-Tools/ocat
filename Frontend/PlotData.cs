//
// Copyright(c) 2018 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
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

using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.IO;
using System.Windows;
using OxyPlot;
using OxyPlot.Series;

namespace Frontend
{
    public struct Session
    {
        public string path;
        public List<double> times;
        public List<double> frameTimes;
        public List<double> misses;
        // TODO: replace with correct data, atm just dummy data
        public List<double> reprojections;
    }

    public class PlotData : INotifyPropertyChanged
    {
        private GraphType type = GraphType.Frametimes;
        public GraphType Type
        {
            get { return type; }
            set
            {
                type = value;
                this.NotifyPropertyChanged("GraphType");
            }
        }

        private string addSessionPath;
        private string removeSessionPath;

        public List<string> csvPaths = new List<string>();
        private PlotModel graph;
        List<Session> sessions = new List<Session>();

        public PlotData() {}

        public void SaveSvg()
        {
            using (var stream = File.Create("test.svg"))
            {
                var exporter = new SvgExporter { Width = 1200, Height = 800 };
                exporter.Export(graph, stream);

                MessageBox.Show("Graph saved as test.svg", "Notification", MessageBoxButton.OK);
            }
        }

        public void LoadData(string csvFile)
        {
            foreach (string path in csvPaths)
            {
                if (path == csvFile)
                {
                    MessageBox.Show("Session already loaded.", "Error", MessageBoxButton.OK);
                    return;
                }
            }
            
            Session session;
            session.path = csvFile;
            session.times = new List<double>();
            session.frameTimes = new List<double>();
            session.misses = new List<double>();
            session.reprojections = new List<double>();

            try
            {
                using (var reader = new StreamReader(csvFile))
                {
                    // header
                    var line = reader.ReadLine();
                    while (!reader.EndOfStream)
                    {
                        line = reader.ReadLine();
                        var values = line.Split(',');

                        // last line can contain warning message of presentMon
                        if (15 < values.Count())
                        {
                            // Just consider DXGI events yet;
                            if (values[3] != "Compositor")
                            {
                                // use dummy data for reprojection
                                double time, frameTime, miss, reprojection;
                                if (double.TryParse(values[11], out time)
                                    && double.TryParse(values[12], out frameTime)
                                    && double.TryParse(values[10], out miss)
                                    && double.TryParse(values[15], out reprojection))
                                {
                                    session.times.Add(time);
                                    session.frameTimes.Add(frameTime);
                                    session.misses.Add(miss);
                                    session.reprojections.Add(reprojection);
                                } else
                                {
                                    MessageBox.Show("File has wrong format.", "Error", MessageBoxButton.OK);
                                    return;
                                }
                            }
                        }
                    }
                }
            }
            catch (IOException)
            {
                MessageBox.Show("Could not access file.", "Error", MessageBoxButton.OK);
                return;
            }

            sessions.Add(session);
            this.NotifyPropertyChanged("CsvPath");

            UpdateGraph();
            csvPaths.Add(csvFile);
        }

        public void UnloadData()
        {
            for (int i = 0; i < csvPaths.Count(); i++)
            {
                if (csvPaths[i] == RemoveSessionPath)
                {
                    csvPaths.RemoveAt(i);
                    sessions.RemoveAt(i);
                    UpdateGraph();
                    return;
                }
            }

            this.NotifyPropertyChanged("CsvPaths");
        }

        public void UpdateGraph()
        {
            PlotModel model = new PlotModel();
            switch(type)
            {
                case (GraphType.Frametimes):
                {
                    model.Title = "Frame times";
                    foreach (Session session in sessions)
                    {
                        LineSeries series = new LineSeries();
                        for (var i = 0; i < session.times.Count; i++)
                        {
                            series.Points.Add(new DataPoint(session.times[i], session.frameTimes[i]));
                        }
                        model.Series.Add(series);
                    }
                    break;
                }
                case (GraphType.Misses):
                {
                    model.Title = "Missed frames";
                    foreach (Session session in sessions)
                    {
                        LineSeries series = new LineSeries();
                        for (var i = 0; i < session.times.Count; i++)
                        {
                            series.Points.Add(new DataPoint(session.times[i], session.misses[i]));
                        }
                        model.Series.Add(series);
                    }
                    break;
                }
                case (GraphType.Reprojections):
                {
                    model.Title = "Reprojections";
                    foreach (Session session in sessions)
                    {
                        LineSeries series = new LineSeries();
                        for (var i = 0; i < session.times.Count; i++)
                        {
                            series.Points.Add(new DataPoint(session.times[i], session.reprojections[i]));
                        }
                        model.Series.Add(series);
                    }
                    break;
                }
            }

            Graph = model;
        }

        public string AddSessionPath
        {
            get { return addSessionPath; }
            set
            {
                addSessionPath = value;
                this.NotifyPropertyChanged("AddSessionPath");
            }
        }

        public string RemoveSessionPath
        {
            get { return removeSessionPath; }
            set
            {
                removeSessionPath = value;
                this.NotifyPropertyChanged("RemoveSessionPath");
            }
        }

        public PlotModel Graph
        {
            get { return graph; }
            set
            {
                graph = value;
                this.NotifyPropertyChanged("Graph");
            }
        }

        public List<string> CsvPaths
        {
            get { return csvPaths; }
            set
            {
                csvPaths = value;
                this.NotifyPropertyChanged("CsvPaths");
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;

        public void NotifyPropertyChanged(string propName)
        {
            if (this.PropertyChanged != null)
            {
                this.PropertyChanged(this, new PropertyChangedEventArgs(propName));
            }
        }
    }
}
