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
        public List<double> frameStart;
        public List<double> frameEnd;
        public List<double> frameTimes;
        public List<double> reprojectionStart;
        public List<double> reprojectionEnd;
        public List<double> reprojectionTimes;
        public List<double> vSync;
        public List<bool> appMissed;
        public List<bool> warpMissed;

        public int appMissesCount;
        public int warpMissesCount;

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

    public class TupleList<T1, T2> : List<Tuple<T1, T2>>
    {
        public void Add( T1 item1, T2 item2)
        {
            Add(new Tuple<T1, T2>(item1, item2));
        }
    }

    public class PlotData : INotifyPropertyChanged
    {
        private GraphType type = GraphType.Frametimes;
        private List<Session> sessions = new List<Session>();
        private Session selectedSession;

        private string addSessionPath;
        private int selectedIndex = -1;
        private int displayedIndex = -1;
        // used to avoid reloading frame detail graph in case our old one is still valid
        private int savedframedetailIndex = -1;

        private List<string> csvPaths = new List<string>();
        private List<string> fileNames = new List<string>();
        private TupleList<int, int> frameRanges = new TupleList<int, int>();

        private PlotModel graph;
        private PlotModel frametimesGraph;
        private PlotModel reprojectionGraph;
        private PlotModel missedframesGraph;
        private PlotModel framedetailGraph;

        private bool readyToLoadSession = false;
        private bool sessionSelected = false;
        private bool enableFrameDetail = false;

        private string frameRange;

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
            SessionSelected = (SelectedIndex >= 0);
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
            session.color = new SolidColorBrush();

            session.frameStart = new List<double>();
            session.frameEnd = new List<double>();
            session.frameTimes = new List<double>();
            session.reprojectionStart = new List<double>();
            session.reprojectionEnd = new List<double>();
            session.reprojectionTimes = new List<double>();
            session.vSync = new List<double>();
            session.appMissed = new List<bool>();
            session.warpMissed = new List<bool>();

            session.appMissesCount = 0;
            session.warpMissesCount = 0;

            try
            {
                using (var reader = new StreamReader(csvFile))
                {
                    // header -> csv layout may differ, identify correct columns based on column title
                    var line = reader.ReadLine();
                    int indexFrameStart = 0;
                    int indexFrameTimes = 0;
                    int indexFrameEnd = 0;
                    int indexReprojectionStart = 0;
                    int indexReprojectionTimes = 0;
                    int indexReprojectionEnd = 0;
                    int indexVSync = 0;
                    int indexAppMissed = 0;
                    int indexWarpMissed = 0;

                    var metrics = line.Split(',');
                    for (int i = 0; i < metrics.Count(); i++)
                    {
                        if (String.Compare(metrics[i], "AppRenderStart") == 0 || String.Compare(metrics[i], "TimeInSeconds") == 0)
                        {
                            indexFrameStart = i;
                        }
                        if (String.Compare(metrics[i], "AppRenderEnd") == 0)
                        {
                            indexFrameEnd = i;
                        }
                        if (String.Compare(metrics[i], "MsBetweenAppPresents") == 0 || String.Compare(metrics[i], "MsBetweenPresents") == 0)
                        {
                            indexFrameTimes = i;
                        }
                        if (String.Compare(metrics[i], "ReprojectionStart") == 0)
                        {
                            indexReprojectionStart = i;
                        }
                        if (String.Compare(metrics[i], "ReprojectionEnd") == 0)
                        {
                            indexReprojectionEnd = i;
                        }
                        if (String.Compare(metrics[i], "MsBetweenReprojections") == 0 || String.Compare(metrics[i], "MsBetweenLsrs") == 0)
                        {
                            indexReprojectionTimes = i;
                        }
                        if (String.Compare(metrics[i], "VSync") == 0)
                        {
                            indexVSync = i;
                        }
                        if (String.Compare(metrics[i], "AppMissed") == 0)
                        {
                            indexAppMissed = i;
                        }
                        if (String.Compare(metrics[i], "WarpMissed") == 0 || String.Compare(metrics[i], "LsrMissed") == 0)
                        {
                            indexWarpMissed = i;
                        }
                    }

                    while (!reader.EndOfStream)
                    {
                        line = reader.ReadLine();
                        var values = line.Split(',');

                        // last row may contain warning message
                        if (values.Count() != metrics.Count())
                            break;

                        // non VR titles only have app render start and frame times metrics
                        if (double.TryParse(values[indexFrameStart], out var frameStart)
                            && double.TryParse(values[indexFrameTimes], out var frameTimes))
                        {
                            session.frameStart.Add(frameStart);
                            session.frameTimes.Add(frameTimes);
                        }

                        if (double.TryParse(values[indexFrameEnd], out var frameEnd)
                         && double.TryParse(values[indexReprojectionStart], out var reprojectionStart)
                         && double.TryParse(values[indexReprojectionTimes], out var reprojectionTimes)
                         && double.TryParse(values[indexReprojectionEnd], out var reprojectionEnd)
                         && double.TryParse(values[indexVSync], out var vSync)
                         && int.TryParse(values[indexAppMissed], out var appMissed)
                         && int.TryParse(values[indexWarpMissed], out var warpMissed))
                        {
                            session.frameEnd.Add(frameEnd);
                            session.reprojectionStart.Add(reprojectionStart);
                            session.reprojectionTimes.Add(reprojectionTimes);
                            session.reprojectionEnd.Add(reprojectionEnd);
                            session.vSync.Add(vSync);
                            session.appMissed.Add(Convert.ToBoolean(appMissed));
                            session.warpMissed.Add(Convert.ToBoolean(warpMissed));
                            session.appMissesCount += appMissed;
                            session.warpMissesCount += warpMissed;
                        }
                    }
                }
            }
            catch (IOException)
            {
                MessageBox.Show("Could not access file.", "Error", MessageBoxButton.OK);
                return;
            }

            if (session.frameStart.Count() == 0)
            {
                // looks like we did not capture any metrics here we support for visualizing
                MessageBox.Show("Wrong format. Did not save any metrics for visualization.", "Error", MessageBoxButton.OK);
            }

            Sessions.Add(session);
            // frame range size is about 500
            if (session.frameStart.Count() <= 600)
            {
                frameRanges.Add(0, (session.frameStart.Count() - 1));
            } else
            {
                frameRanges.Add(0, 500);
            }
            UpdateGraphs();
            UpdateColorIdentifier();
            CsvPaths.Add(csvFile);
            FileNames.Add(session.Filename);
        }

        public void UnloadData()
        {
            if (SelectedIndex >= 0 && SelectedIndex < Sessions.Count())
            {
                if (displayedIndex != -1 && displayedIndex > SelectedIndex)
                {
                    displayedIndex--;
                    savedframedetailIndex--;
                } else if (displayedIndex == SelectedIndex)
                {
                    displayedIndex = -1;
                    savedframedetailIndex = -1;
                }
                CsvPaths.RemoveAt(SelectedIndex);
                Sessions.RemoveAt(SelectedIndex);
                FileNames.RemoveAt(SelectedIndex);
                frameRanges.RemoveAt(SelectedIndex);
                SelectedIndex = -1;
                UpdateGraphs();
                UpdateColorIdentifier();
            }
        }

        public void UpdateGraphs()
        {
            switch (type)
            {
                case (GraphType.Frametimes):
                {
                    UpdateMissedFramesStats();
                    UpdateReprojectionTimes();
                    UpdateFrameTimes();
                    break;
                }
                case (GraphType.Misses):
                {
                    UpdateFrameTimes();
                    UpdateReprojectionTimes();
                    UpdateMissedFramesStats();
                    break;
                }
                case (GraphType.Reprojections):
                {
                    UpdateMissedFramesStats();
                    UpdateFrameTimes();
                    UpdateReprojectionTimes();
                    break;
                }
                case (GraphType.FrameDetail):
                {
                    UpdateMissedFramesStats();
                    UpdateReprojectionTimes();
                    UpdateFrameTimes();
                    ShowFrameEvents();
                    return;
                }
            }

            displayedIndex = -1;
            EnableFrameDetail = SessionSelected;
        }

        public static DataPoint myDelegate(object inputData)
        {
            var data = (EventDataPoint)inputData;
            return (new DataPoint(data.x, data.y));
        }

        private void UpdateFrameTimes()
        {
            System.Func<object, DataPoint> handler = myDelegate;
            PlotModel model = new PlotModel();
            model.Title = "Frame times";
            for (var iSession = 0; iSession < Sessions.Count; iSession++)
            {
                LineSeries series = new LineSeries();

                series.Mapping = handler;

                List<EventDataPoint> frame = new List<EventDataPoint>();

                for (var i = 0; i < Sessions[iSession].frameStart.Count; i++)
                {
                    if (Sessions[iSession].frameTimes[i] != 0)
                    {
                        frame.Add(new EventDataPoint(Sessions[iSession].frameStart[i], Sessions[iSession].frameTimes[i], sessions[iSession].filename + "\nFrame " + i));
                    } 
                }

                series.ItemsSource = frame;

                series.ToolTip = sessions[iSession].filename;
                series.TrackerFormatString = "{EventTitle}\nTime: {2:0.###}\nFrame time (ms): {4:0.###}";

                model.Series.Add(series);
            }
            Graph = model;
            frametimesGraph = model;
            Type = GraphType.Frametimes;
        }

        private void UpdateReprojectionTimes()
        {
            System.Func<object, DataPoint> handler = myDelegate;
            PlotModel model = new PlotModel();
            model.Title = "Reprojections";
            for (var iSession = 0; iSession < Sessions.Count; iSession++)
            {
                LineSeries series = new LineSeries();

                ScatterSeries scatterApp = new ScatterSeries();
                scatterApp.MarkerType = MarkerType.Circle;
                scatterApp.MarkerFill = OxyColors.Orange;
                if (iSession == 0)
                    scatterApp.Title = "App misses";

                ScatterSeries scatterWarp = new ScatterSeries();
                scatterWarp.MarkerType = MarkerType.Circle;
                scatterWarp.MarkerFill = OxyColors.Red;
                if (iSession == 0)
                    scatterWarp.Title = "Warp misses";

                series.Mapping = handler;

                List<EventDataPoint> frame = new List<EventDataPoint>();

                for (var i = 0; i < Sessions[iSession].reprojectionStart.Count; i++)
                {
                    if (Sessions[iSession].reprojectionTimes[i] != 0)
                    {
                        frame.Add(new EventDataPoint(Sessions[iSession].reprojectionStart[i], Sessions[iSession].reprojectionTimes[i], sessions[iSession].filename + "\nFrame " + i));
                        // only show misses of reprojections if we have valid data
                        if (Sessions[iSession].warpMissed[i])
                        {
                            scatterWarp.Points.Add(new ScatterPoint(Sessions[iSession].reprojectionStart[i], Sessions[iSession].reprojectionTimes[i], 2));
                        }

                        if (Sessions[iSession].appMissed[i])
                        {
                            scatterApp.Points.Add(new ScatterPoint(Sessions[iSession].reprojectionStart[i], Sessions[iSession].reprojectionTimes[i], 2));
                        }
                    }
                }

                series.ItemsSource = frame;

                series.ToolTip = sessions[iSession].filename;
                series.TrackerFormatString = "{EventTitle}\nTime: {2:0.###}\nFrame time (ms): {4:0.###}";

                model.Series.Add(series);
                model.Series.Add(scatterApp);
                model.Series.Add(scatterWarp);

                model.IsLegendVisible = true;
                model.LegendItemOrder = LegendItemOrder.Reverse;
                model.LegendPlacement = LegendPlacement.Inside;
                model.LegendPosition = LegendPosition.RightTop;
            }
            Graph = model;
            reprojectionGraph = model;
            Type = GraphType.Reprojections;
        }

        private void UpdateMissedFramesStats()
        {
            PlotModel model = new PlotModel();
            model.Title = "Missed frames";

            ColumnSeries successFrames = new ColumnSeries();
            successFrames.IsStacked = true;
            ColumnSeries missedAppFrames = new ColumnSeries();
            missedAppFrames.IsStacked = true;
            missedAppFrames.Title = "App misses";
            missedAppFrames.FillColor = OxyColors.Orange;
            ColumnSeries missedWarpFrames = new ColumnSeries();
            missedWarpFrames.IsStacked = true;
            missedWarpFrames.Title = "Warp misses";
            missedWarpFrames.FillColor = OxyColors.Red;

            var categoryAxis = new CategoryAxis();
            categoryAxis.IsAxisVisible = false;

            for (var iSession = 0; iSession < Sessions.Count; iSession++)
            {

                successFrames.Items.Add(new ColumnItem(
                    ((double)(Sessions[iSession].appMissed.Count()
                    - Sessions[iSession].appMissesCount - sessions[iSession].warpMissesCount)
                    / Sessions[iSession].appMissed.Count()) * 100, iSession));
                missedAppFrames.Items.Add(new ColumnItem(((double)Sessions[iSession].appMissesCount
                    / Sessions[iSession].appMissed.Count()) * 100, iSession));
                missedWarpFrames.Items.Add(new ColumnItem(((double)Sessions[iSession].warpMissesCount
                    / Sessions[iSession].appMissed.Count()) * 100, iSession));

                categoryAxis.Labels.Add(Sessions[iSession].filename);
            }

            missedAppFrames.LabelPlacement = LabelPlacement.Middle;
            missedAppFrames.LabelFormatString = "{0:.00}%";
            missedWarpFrames.LabelPlacement = LabelPlacement.Outside;
            missedWarpFrames.LabelFormatString = "{0:.00}%";

            model.Series.Add(successFrames);
            model.Series.Add(missedAppFrames);
            model.Series.Add(missedWarpFrames);

            LinearAxis linearAxis = new LinearAxis();
            linearAxis.AbsoluteMinimum = 0;
            linearAxis.MaximumPadding = 0.06;
            linearAxis.Title = "% total frames";

            model.Axes.Clear();
            model.Axes.Add(categoryAxis);
            model.Axes.Add(linearAxis);

            model.IsLegendVisible = true;
            model.LegendPosition = LegendPosition.RightTop;
            model.LegendPlacement = LegendPlacement.Outside;
            model.LegendItemOrder = LegendItemOrder.Reverse;
            model.PlotAreaBorderThickness = new OxyThickness(1, 0, 0, 1);

            Graph = model;
            missedframesGraph = model;
            Type = GraphType.Misses;
        }

        public void ShowFrameTimes()
        {
            Type = GraphType.Frametimes;
            Graph = frametimesGraph;
            displayedIndex = -1;
            EnableFrameDetail = SessionSelected;
        }

        public void ShowReprojectionTimes()
        {
            Type = GraphType.Reprojections;
            Graph = reprojectionGraph;
            displayedIndex = -1;
            EnableFrameDetail = SessionSelected;
        }

        public void ShowMissedFramesTimes()
        {
            Type = GraphType.Misses;
            Graph = missedframesGraph;
            displayedIndex = -1;
            EnableFrameDetail = SessionSelected;
        }

        public void JumpBackFrames()
        {
            if (frameRanges[SelectedIndex].Item1 > 0)
            {
                frameRanges[SelectedIndex] = Tuple.Create(frameRanges[SelectedIndex].Item1 - 400, frameRanges[SelectedIndex].Item1 + 100);
                savedframedetailIndex = -1;
            }

            ShowFrameEvents();
        }

        public void JumpForwardFrames()
        {
            if ((sessions[SelectedIndex].frameStart.Count() - 1) <= frameRanges[SelectedIndex].Item2 + 500
                && (sessions[SelectedIndex].frameStart.Count() - 1) > frameRanges[SelectedIndex].Item2)
            {
                frameRanges[SelectedIndex] = Tuple.Create(frameRanges[SelectedIndex].Item1 + 400, sessions[SelectedIndex].frameStart.Count() - 1);
                savedframedetailIndex = -1;
            } else if ((sessions[SelectedIndex].frameStart.Count() - 1) > frameRanges[SelectedIndex].Item2)
            {
                frameRanges[SelectedIndex] = Tuple.Create(frameRanges[SelectedIndex].Item1 + 400, frameRanges[SelectedIndex].Item2 + 400);
                savedframedetailIndex = -1;
            }

            ShowFrameEvents();
        }

        public void ShowFrameEvents()
        {
            // can't update if no session is selected, just keep current graph
            if (!SessionSelected)
            {
                savedframedetailIndex = -1;
                return;
            }

            if (savedframedetailIndex == SelectedIndex)
            {
                Type = GraphType.FrameDetail;
                Graph = framedetailGraph;
                displayedIndex = savedframedetailIndex;
                EnableFrameDetail = false;
                return;
            }

            System.Func<object, DataPoint> handler = myDelegate;
            PlotModel model = new PlotModel();
            int lineRow = 0;
            double xAxisMinimum = 0;
            double xAxisMaximum = 0;

            var vSyncIndicators = new StemSeries
            {
                MarkerType = MarkerType.Circle
            };

            if (Sessions[SelectedIndex].vSync.Count() > 0)
            {
                // limit displayed frames to 500 due to performance issues if too many frames/series are loaded into graph
                for (var i = frameRanges[SelectedIndex].Item1; i < Sessions[SelectedIndex].frameStart.Count() && i <= frameRanges[SelectedIndex].Item2; i++)
                {
                    vSyncIndicators.Points.Add(new DataPoint(Sessions[SelectedIndex].vSync[i], 300));

                    LineSeries series = new LineSeries();

                    series.Mapping = handler;

                    List<EventDataPoint> frame = new List<EventDataPoint>();
                    if (Sessions[SelectedIndex].frameStart[i] != 0)
                    {
                        frame.Add(new EventDataPoint(Sessions[SelectedIndex].frameStart[i], lineRow + 106, "Start frame (App)"));
                    }
                    if (Sessions[SelectedIndex].frameEnd[i] != 0)
                    {
                        frame.Add(new EventDataPoint(Sessions[SelectedIndex].frameEnd[i], lineRow + 106, "End frame (App)"));
                    }
                    if (Sessions[SelectedIndex].reprojectionStart[i] != 0)
                    {
                        frame.Add(new EventDataPoint(Sessions[SelectedIndex].reprojectionStart[i], lineRow + 106, "Start Reprojection"));
                    }
                    if (Sessions[SelectedIndex].reprojectionEnd[i] != 0)
                    {
                        frame.Add(new EventDataPoint(Sessions[SelectedIndex].reprojectionEnd[i], lineRow + 106, "End Reprojection"));
                    }

                    series.ItemsSource = frame;

                    // at the beginning we jump to the first few frames, axis values depend on the specific session
                    if (xAxisMinimum == 0)
                    {
                        xAxisMinimum = Sessions[SelectedIndex].frameStart[i];
                    }

                    if (i <= frameRanges[SelectedIndex].Item1 + 3 && xAxisMaximum < Sessions[SelectedIndex].reprojectionEnd[i])
                        xAxisMaximum = Sessions[SelectedIndex].reprojectionEnd[i];

                    // toggle between two rows to prevent too much visual overlap of the frames
                    lineRow = (lineRow + 1) % 2;

                    series.MarkerType = MarkerType.Circle;
                    series.ToolTip = "Frame" + i;
                    series.Title = "Frame " + i;

                    if (Sessions[SelectedIndex].appMissed[i])
                    {
                        series.MarkerFill = OxyColors.Orange;
                        series.Color = OxyColors.Orange;
                        series.ToolTip += " - App Miss";
                        series.Title += " - App Miss";
                    }
                    else if (Sessions[SelectedIndex].warpMissed[i])
                    {
                        series.MarkerFill = OxyColors.Red;
                        series.Color = OxyColors.Red;
                        series.ToolTip += " - Warp Miss";
                        series.Title += " - Warp Miss";
                    }
                    else
                    {
                        series.MarkerFill = OxyColor.FromRgb(125, 125, 125);
                        series.Color = OxyColor.FromRgb(0, 0, 0);
                    }
                    
                    series.TrackerFormatString = "{0}\n{EventTitle}: {2:0.###}";
                    series.CanTrackerInterpolatePoints = false;

                    model.Series.Add(series);
                }
            }

            model.Series.Add(vSyncIndicators);

            double dis = xAxisMaximum - xAxisMinimum;
            dis = dis * 0.1;
            xAxisMaximum += dis;
            xAxisMinimum -= dis;

            LinearAxis xAxis = new LinearAxis();
            xAxis.Minimum = xAxisMinimum;
            xAxis.Maximum = xAxisMaximum;
            xAxis.Position = AxisPosition.Bottom;
            xAxis.Title = "Timestamp of events";

            xAxis.MinimumRange = 0.01;
            xAxis.MaximumRange = 1.0;

            LinearAxis yAxis = new LinearAxis();
            yAxis.Minimum = 103;
            yAxis.Maximum = 110;
            yAxis.Position = AxisPosition.Left;
            yAxis.IsAxisVisible = false;

            yAxis.MinimumRange = 2.0;
            yAxis.MaximumRange = 4.0;

            model.Axes.Clear();
            model.Axes.Add(xAxis);
            model.Axes.Add(yAxis);

            model.Title = "Frame events";
            model.Subtitle = sessions[SelectedIndex].Filename;
            model.IsLegendVisible = false;

            Graph = model;
            displayedIndex = SelectedIndex;
            EnableFrameDetail = false;

            framedetailGraph = model;
            savedframedetailIndex = SelectedIndex;
            Type = GraphType.FrameDetail;

            FrameRange = " " + frameRanges[SelectedIndex].Item1 + " - " + frameRanges[SelectedIndex].Item2 + " ";
        }

        public void UpdateColorIdentifier()
        {
            IList<OxyColor> colorList = Graph.DefaultColors;
            for (int iSession = 0; iSession < Sessions.Count(); iSession++)
            {
                sessions[iSession].color.Color = Color.FromRgb(
                    colorList.ElementAt(iSession).R,
                    colorList.ElementAt(iSession).G,
                    colorList.ElementAt(iSession).B);
            }
        }

        public GraphType Type
        {
            get { return type; }
            set
            {
                type = value;
                this.NotifyPropertyChanged("Type");
            }
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

        public Session SelectedSession
        {
            get { return selectedSession; }
            set
            {
                selectedSession = value;
                this.NotifyPropertyChanged("SelectedSession");
            }
        }

        public int SelectedIndex
        {
            get { return selectedIndex; }
            set
            {
                selectedIndex = value;
                this.NotifyPropertyChanged("SelectedIndex");
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
                } else
                {
                    EnableFrameDetail = !(displayedIndex == selectedIndex);
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

        public string FrameRange
        {
            get { return frameRange; }
            set
            {
                frameRange = value;
                this.NotifyPropertyChanged("FrameRange");
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
