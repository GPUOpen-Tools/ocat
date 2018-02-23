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

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.IO;
using System.Windows;
using System.Windows.Media;
using OxyPlot;
using OxyPlot.Series;
using OxyPlot.Axes;

namespace Frontend
{
    public struct Session
    {
        public string path;
        public string filename;
        public List<double> times;
        public List<double> frameTimes;
        public List<double> misses;
        // TODO: replace with correct data, atm just dummy data
        public List<double> reprojections;

        // per frame analysis
        public List<double> renderTimes;
        public List<double> displayTimes;

        public SolidColorBrush color;

        public string Filename
        {
            get { return filename; }
            set
            {
                filename = value;
            }
        }

        public SolidColorBrush Color
        {
            get { return color; }
            set
            {
                color = value;
            }
        }
    }


    delegate DataPoint myMapping<in EventDataPoint, out DataPoint> (EventDataPoint myItem);

    public class PlotData : INotifyPropertyChanged
    {
        private GraphType type = GraphType.Frametimes;
        public GraphType Type
        {
            get { return type; }
            set
            {
                type = value;
                this.NotifyPropertyChanged("Type");
            }
        }

        private string addSessionPath;
        public Session removeSessionPath;
        private int removeIndex = -1;
        private int displayedIndex = -1;

        public List<string> csvPaths = new List<string>();
        public List<string> fileNames = new List<string>();
        private PlotModel graph;
        List<Session> sessions = new List<Session>();

        private bool readyToLoadSession = false;
        private bool sessionSelected = false;

        private bool enableFrameDetail = false;
        public PlotData() {}

        public void SaveSvg(string path)
        {
            using (var stream = File.Create(path))
            {
                var exporter = new SvgExporter { Width = 1200, Height = 800 };
                exporter.Export(graph, stream);

                MessageBox.Show("Graph saved.", "Notification", MessageBoxButton.OK);
            }
        }

        public void UpdateSelectionIndex()
        {
            if (RemoveIndex >= 0)
            {
                SessionSelected = true;
            } else
            {
                SessionSelected = false;
            }
        }

        public void LoadData(string csvFile)
        {
            if (csvFile == null || csvFile == "")
            {
                MessageBox.Show("Please select a session.", "Error", MessageBoxButton.OK);
                return;
            }

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

            int index = csvFile.LastIndexOf('\\');
            session.filename = csvFile.Substring(index + 1);

            session.times = new List<double>();
            session.frameTimes = new List<double>();
            session.misses = new List<double>();
            session.reprojections = new List<double>();
            session.renderTimes = new List<double>();
            session.displayTimes = new List<double>();

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
                        if (10 < values.Count())
                        {
                            double time, frameTime, miss, reprojection, renderTime, displayTime;
                            if (double.TryParse(values[4], out time)
                                && double.TryParse(values[2], out frameTime)
                                && double.TryParse(values[9], out miss)
                                && double.TryParse(values[3], out reprojection)
                                && double.TryParse(values[5], out renderTime)
                                && double.TryParse(values[8], out displayTime))
                            {
                                if (time != 0)
                                {
                                    session.times.Add(time);
                                    session.frameTimes.Add(frameTime);
                                    session.misses.Add(miss);
                                    session.reprojections.Add(reprojection);
                                    session.renderTimes.Add(renderTime);
                                    session.displayTimes.Add(displayTime);
                                }
                            } else
                            {
                                MessageBox.Show("File has wrong format.", "Error", MessageBoxButton.OK);
                                return;
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

            session.color = new SolidColorBrush();

            session.color.Color = Color.FromRgb(255, 0, 0);

            sessions.Add(session);

            UpdateGraph();
            csvPaths.Add(csvFile);
            fileNames.Add(session.filename);
            this.NotifyPropertyChanged("CsvPaths");
            this.NotifyPropertyChanged("FileNames");
            this.NotifyPropertyChanged("Sessions");
        }

        public void UnloadData()
        {
            if (RemoveIndex >= 0 && RemoveIndex < Sessions.Count())
            {
                csvPaths.RemoveAt(RemoveIndex);
                sessions.RemoveAt(RemoveIndex);
                fileNames.RemoveAt(RemoveIndex);
                UpdateGraph();

                this.NotifyPropertyChanged("FileNames");
                this.NotifyPropertyChanged("CsvPaths");
                this.NotifyPropertyChanged("Sessions");

                RemoveIndex = -1;
            }
        }

        public void UpdateGraph()
        {
            PlotModel model = new PlotModel();
            IList<OxyColor> colorList = model.DefaultColors;
            switch (type)
            {
                case (GraphType.Frametimes):
                {
                    model.Title = "Frame times";
                    for (var iSession = 0; iSession < sessions.Count; iSession++)
                    {
                        LineSeries series = new LineSeries();
                        for (var i = 0; i < sessions[iSession].times.Count; i++)
                        {
                            series.Points.Add(new DataPoint(sessions[iSession].times[i], sessions[iSession].frameTimes[i]));
                        }

                        series.ToolTip = sessions[iSession].filename;

                        model.Series.Add(series);

                        sessions[iSession].color.Color = Color.FromRgb(
                            colorList.ElementAt(iSession).R,
                            colorList.ElementAt(iSession).G,
                            colorList.ElementAt(iSession).B);
                    }
                    break;
                }
                case (GraphType.Misses):
                {
                    model.Title = "Missed frames";
                    for (var iSession = 0; iSession < sessions.Count; iSession++)
                    {
                        LineSeries series = new LineSeries();
                        for (var i = 0; i < sessions[iSession].times.Count; i++)
                        {
                            series.Points.Add(new DataPoint(sessions[iSession].times[i], sessions[iSession].misses[i]));
                        }

                        series.ToolTip = sessions[iSession].filename;

                        model.Series.Add(series);

                        sessions[iSession].color.Color = Color.FromRgb(
                            colorList.ElementAt(iSession).R,
                            colorList.ElementAt(iSession).G,
                            colorList.ElementAt(iSession).B);
                    }

                    break;
                }
                case (GraphType.Reprojections):
                {
                    model.Title = "Reprojections";
                    for (var iSession = 0; iSession < sessions.Count; iSession++)
                    {
                        LineSeries series = new LineSeries();
                        for (var i = 0; i < sessions[iSession].times.Count; i++)
                        {
                            series.Points.Add(new DataPoint(sessions[iSession].times[i], sessions[iSession].reprojections[i]));
                        }

                        series.ToolTip = sessions[iSession].filename;

                        model.Series.Add(series);

                        sessions[iSession].color.Color = Color.FromRgb(
                            colorList.ElementAt(iSession).R,
                            colorList.ElementAt(iSession).G,
                            colorList.ElementAt(iSession).B);
                    }

                    break;
                }
                case (GraphType.FrameDetail):
                {
                    ShowFrameEvents();
                    return;
                }
            }

            displayedIndex = -1;
            if (SessionSelected)
            {
                EnableFrameDetail = true;
            } else
            {
                EnableFrameDetail = false;
            }
            Graph = model;
            this.NotifyPropertyChanged("Sessions");
        }

        public static DataPoint myDel(object inputData)
        {
            var data = (EventDataPoint)inputData;
            return (new DataPoint(data.x, data.y));
        }

        public void ShowFrameEvents()
        {
            // we can't just show every captured frame
            // if session contains too many frames, performance is highly decreased so just display first xxx frames
            System.Func<object, DataPoint> handler = myDel;

            PlotModel model = new PlotModel();

            int line = 0;

            int index = 0;

            double xMin = 0;
            double xMax = 0;

            if (Sessions.Count() <= 0)
            {

                MessageBox.Show("No sessions loaded.", "Error", MessageBoxButton.OK);
                return;
            }

            if (SessionSelected)
            {
                index = RemoveIndex;
            }
            else
            {
                return;
            }

            var stemSeries = new StemSeries
            {
                MarkerType = MarkerType.Circle
            };

            for (var i = 0; i < sessions[index].times.Count() && i < 500; i++)
            {
                if (1.0 == sessions[index].misses[i])
                {
                    LineSeries series = new LineSeries();

                    series.Mapping = handler;

                    series.ItemsSource = new List<EventDataPoint>(new[]
                    {
                        new EventDataPoint(sessions[index].times[i], line + 3, "Start frame"),
                        new EventDataPoint(sessions[index].renderTimes[i], line + 3, "Render"),
                    });

                    if (i == 0)
                        xMin = sessions[index].times[i];

                    if (i <= 3)
                        xMax = sessions[index].renderTimes[i];

                    line = (line + 1) % 2;

                    series.MarkerFill = OxyColor.FromRgb(255, 0, 0);
                    series.Color = OxyColor.FromRgb(255, 0, 0);
                    series.MarkerType = MarkerType.Circle;

                    series.ToolTip = "Frame" + i;
                    series.Title = "Frame " + i;
                    series.TrackerFormatString = "{0}\n{EventTitle}: {2:0.###}";
                    series.CanTrackerInterpolatePoints = false;

                    model.Series.Add(series);
                }
                else
                {
                    stemSeries.Points.Add(new DataPoint(sessions[index].displayTimes[i], 100));

                    LineSeries series = new LineSeries();

                    series.Mapping = handler;

                    series.ItemsSource = new List<EventDataPoint>(new[]
                    {
                        new EventDataPoint(sessions[index].times[i], line + 3, "Start frame"),
                        new EventDataPoint(sessions[index].renderTimes[i], line + 3, "Render"),
                        new EventDataPoint(sessions[index].displayTimes[i], line + 3, "Display frame")
                    });


                    if (i == 0)
                        xMin = sessions[index].times[i];

                    if (i <= 3)
                        xMax = sessions[index].displayTimes[i];

                    line = (line + 1) % 2;

                    series.MarkerFill = OxyColor.FromRgb(125, 125, 125);
                    series.Color = OxyColor.FromRgb(0, 0, 0);
                    series.MarkerType = MarkerType.Circle;

                    series.ToolTip = "Frame" + i;
                    series.Title = "Frame " + i;
                    series.TrackerFormatString = "{0}\n{EventTitle}: {2:0.###}";
                    series.CanTrackerInterpolatePoints = false;

                    model.Series.Add(series);   
                }
            }

            model.Series.Add(stemSeries);

            double dis = xMax - xMin;
            dis = dis * 0.1;
            xMax += dis;
            xMin -= dis;

            LinearAxis xAxis = new LinearAxis();
            xAxis.Minimum = xMin;
            xAxis.Maximum = xMax;
            xAxis.Position = AxisPosition.Bottom;
            xAxis.Title = "Timestamp of events";

            LinearAxis yAxis = new LinearAxis();
            yAxis.Minimum = 0;
            yAxis.Maximum = 7;
            yAxis.Position = AxisPosition.Left;
            yAxis.IsAxisVisible = false;

            model.Axes.Clear();
            model.Axes.Add(xAxis);
            model.Axes.Add(yAxis);

            model.Title = "Frame events";
            model.Subtitle = sessions[index].Filename;

            model.IsLegendVisible = false;

            Graph = model;
            this.NotifyPropertyChanged("Sessions");

            displayedIndex = index;
            EnableFrameDetail = false;
        }

        public String AddSessionPath
        {
            get { return addSessionPath; }
            set
            {
                addSessionPath = value;
                if (!String.IsNullOrEmpty(addSessionPath))
                {
                    ReadyToLoadSession = true;
                }
                this.NotifyPropertyChanged("AddSessionPath");
            }
        }

        public Session RemoveSessionPath
        {
            get { return removeSessionPath; }
            set
            {
                removeSessionPath = value;
                this.NotifyPropertyChanged("RemoveSessionPath");
            }
        }

        public int RemoveIndex
        {
            get { return removeIndex; }
            set
            {
                removeIndex = value;
                this.NotifyPropertyChanged("RemoveIndex");
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

        public List<string> FileNames
        {
            get { return fileNames; }
            set
            {
                csvPaths = value;
                this.NotifyPropertyChanged("FileNames");
            }
        }

        public List<Session> Sessions
        {
            get { return sessions; }
            set
            {
                sessions = value;
                this.NotifyPropertyChanged("Sessions");
            }
        }

        public bool ReadyToLoadSession
        {
            get { return readyToLoadSession; }
            set
            {
                readyToLoadSession = value;
                this.NotifyPropertyChanged("ReadyToLoadSession");
            }
        }

        public bool SessionSelected
        {
            get { return sessionSelected; }
            set
            {
                sessionSelected = value;
                if (!sessionSelected)
                {
                    EnableFrameDetail = false;
                } else if (displayedIndex != removeIndex)
                {
                    EnableFrameDetail = true;
                } else if (displayedIndex == removeIndex)
                {
                    EnableFrameDetail = false;
                }
                this.NotifyPropertyChanged("SessionSelected");
            }
        }

        public bool EnableFrameDetail
        {
            get { return enableFrameDetail; }
            set
            {
                enableFrameDetail = value;
                this.NotifyPropertyChanged("EnableFrameDetail");
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
