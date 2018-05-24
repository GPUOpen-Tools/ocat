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

        public bool isVR;
        public int appMissesCount;
        public int warpMissesCount;
        public int validAppFrames;
        public int validReproFrames;
        public double lastFrameTime;
        public double lastReprojectionTime;

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

        private string subType;
        private string additionalOption;
        private Metric metric = Metric.MissedFrames;
        const int maxMetricsNum = 5;

        private bool nextOption = false;
        private bool prevOption = false;

        public PlotData() {}

        public void SavePdf(string path)
        {
            using (var stream = File.Create(path))
            {
                var exporter = new PdfExporter { Width = 1200, Height = 800 };
                exporter.Export(graph, stream);

                //MessageBox.Show("Graph saved.", "Notification", MessageBoxButton.OK);
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

            session.isVR = false;
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
            session.validAppFrames = 0;
            session.lastFrameTime = 0;
            session.validReproFrames = 0;
            session.lastReprojectionTime = 0;

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
                        // MsUntilRenderComplete needs to be added to AppRenderStart to get the timestamp
                        if (String.Compare(metrics[i], "AppRenderEnd") == 0 || String.Compare(metrics[i], "MsUntilRenderComplete") == 0)
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
                        //MsUntilDisplayed needs to be added to AppRenderStart, we don't have a reprojection start timestamp in this case
                        if (String.Compare(metrics[i], "ReprojectionEnd") == 0 || String.Compare(metrics[i], "MsUntilDisplayed") == 0)
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
                            session.isVR = true;
                        }
                        if (String.Compare(metrics[i], "AppMissed") == 0 || String.Compare(metrics[i], "Dropped") == 0)
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
                        // app render end and reprojection end get calculated based on ms until render complete and ms until displayed metric
                        if (double.TryParse(values[indexFrameStart], out var frameStart)
                            && double.TryParse(values[indexFrameTimes], out var frameTimes)
                            && int.TryParse(values[indexAppMissed], out var appMissed))
                        {
                            if (frameStart > 0)
                            {
                                session.validAppFrames++;
                                session.lastFrameTime = frameStart;
                            }
                            session.frameStart.Add(frameStart);
                            session.frameTimes.Add(frameTimes);
                            
                            session.appMissed.Add(Convert.ToBoolean(appMissed));
                            session.appMissesCount += appMissed;
                        }

                        if (double.TryParse(values[indexFrameEnd], out var frameEnd)
                            && double.TryParse(values[indexReprojectionEnd], out var reprojectionEnd))
                        {
                            if (session.isVR)
                            {
                                session.frameEnd.Add(frameEnd);
                                session.reprojectionEnd.Add(reprojectionEnd);
                            }
                            else
                            {
                                session.frameEnd.Add(frameStart + frameEnd / 1000.0);
                                session.reprojectionEnd.Add(frameStart + reprojectionEnd / 1000.0);
                            }
                        }

                        if (double.TryParse(values[indexReprojectionStart], out var reprojectionStart)
                         && double.TryParse(values[indexReprojectionTimes], out var reprojectionTimes)
                         && double.TryParse(values[indexVSync], out var vSync)
                         && int.TryParse(values[indexWarpMissed], out var warpMissed))
                        {
                            if (reprojectionStart > 0)
                            {
                                session.validReproFrames++;
                                session.lastReprojectionTime = reprojectionStart;
                            }
                            session.reprojectionStart.Add(reprojectionStart);
                            session.reprojectionTimes.Add(reprojectionTimes);
                            session.vSync.Add(vSync);
                            session.warpMissed.Add(Convert.ToBoolean(warpMissed));
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
                if (savedframedetailIndex != -1 && savedframedetailIndex > SelectedIndex)
                {
                    displayedIndex--;
                    savedframedetailIndex--;
                } else if (savedframedetailIndex == SelectedIndex)
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
                    UpdateFramesStats();
                    UpdateReprojectionTimes();
                    UpdateFrameTimes();
                    break;
                }
                case (GraphType.Reprojections):
                {
                    UpdateFramesStats();
                    UpdateFrameTimes();
                    UpdateReprojectionTimes();
                    break;
                }
                case (GraphType.Misses):
                {
                    UpdateFrameTimes();
                    UpdateReprojectionTimes();
                    UpdateFramesStats();
                    break;
                }
                case (GraphType.FrameDetail):
                {
                    UpdateFramesStats();
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

            double minValueY = Double.MaxValue;
            double maxValueY = 0.0;

            double minValueX = Double.MaxValue;
            double maxValueX = 0.0;

            for (var iSession = 0; iSession < Sessions.Count; iSession++)
            {
                LineSeries series = new LineSeries();

                series.Mapping = handler;

                List<EventDataPoint> frame = new List<EventDataPoint>();

                double lastFrameStart = 0;

                for (var i = 0; i < Sessions[iSession].frameStart.Count; i++)
                {
                    if (Sessions[iSession].frameTimes[i] != 0)
                    {
                        frame.Add(new EventDataPoint(Sessions[iSession].frameStart[i], Sessions[iSession].frameTimes[i], sessions[iSession].filename + "\nFrame " + i));

                        if (Sessions[iSession].frameStart[i] < minValueX)
                            minValueX = Sessions[iSession].frameStart[i];
                        if (Sessions[iSession].frameStart[i] > maxValueX)
                            maxValueX = Sessions[iSession].frameStart[i];

                        if (Sessions[iSession].frameTimes[i] < minValueY)
                            minValueY = Sessions[iSession].frameTimes[i];
                        if (Sessions[iSession].frameTimes[i] > maxValueY)
                            maxValueY = Sessions[iSession].frameTimes[i];

                        lastFrameStart = Sessions[iSession].frameStart[i];
                    } else if (lastFrameStart != 0 && Sessions[iSession].frameStart[i] != 0)
                    {
                        double frameTime = 1000.0 * (Sessions[iSession].frameStart[i] - lastFrameStart);

                        frame.Add(new EventDataPoint(Sessions[iSession].frameStart[i], frameTime, sessions[iSession].filename + "\nFrame " + i));

                        if (Sessions[iSession].frameStart[i] < minValueX)
                            minValueX = Sessions[iSession].frameStart[i];
                        if (Sessions[iSession].frameStart[i] > maxValueX)
                            maxValueX = Sessions[iSession].frameStart[i];

                        if (frameTime < minValueY)
                            minValueY = frameTime;
                        if (frameTime > maxValueY)
                            maxValueY = frameTime;

                        lastFrameStart = Sessions[iSession].frameStart[i];
                    }         
                }

                series.ItemsSource = frame;

                series.ToolTip = sessions[iSession].filename;
                series.TrackerFormatString = "{EventTitle}\nTime: {2:0.###}\nFrame time (ms): {4:0.###}";

                model.Series.Add(series);
            }

            if (Sessions.Count > 0 && Sessions[0].frameStart.Count > 0)
            {
                LinearAxis linearAxisX = new LinearAxis();
                linearAxisX.Minimum = minValueX;
                linearAxisX.Maximum = maxValueX;
                linearAxisX.Title = "frame start during recording time s";
                linearAxisX.Position = AxisPosition.Bottom;

                LinearAxis linearAxisY = new LinearAxis();
                linearAxisY.Minimum = 0.0;
                linearAxisY.Maximum = maxValueY + minValueY;
                linearAxisY.Title = "frame times ms";
                linearAxisY.Position = AxisPosition.Left;

                model.Axes.Clear();
                model.Axes.Add(linearAxisX);
                model.Axes.Add(linearAxisY);
            }

            if (Sessions.Count == 1)
                model.Subtitle = sessions[0].Filename;

            Graph = model;
            frametimesGraph = model;
            Type = GraphType.Frametimes;
        }

        private void UpdateReprojectionTimes()
        {
            System.Func<object, DataPoint> handler = myDelegate;
            PlotModel model = new PlotModel();
            model.Title = "Reprojections";

            double minValueY = Double.MaxValue;
            double maxValueY = 0.0;

            double minValueX = Double.MaxValue;
            double maxValueX = 0.0;

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

                        if (Sessions[iSession].reprojectionStart[i] < minValueX)
                            minValueX = Sessions[iSession].reprojectionStart[i];
                        if (Sessions[iSession].reprojectionStart[i] > maxValueX)
                            maxValueX = Sessions[iSession].reprojectionStart[i];

                        if (Sessions[iSession].reprojectionTimes[i] < minValueY)
                            minValueY = Sessions[iSession].reprojectionTimes[i];
                        if (Sessions[iSession].reprojectionTimes[i] > maxValueY)
                            maxValueY = Sessions[iSession].reprojectionTimes[i];
                    }
                }

                series.ItemsSource = frame;
                series.ToolTip = sessions[iSession].filename;
                series.TrackerFormatString = "{EventTitle}\nTime: {2:0.###}\nFrame time (ms): {4:0.###}";

                model.Series.Add(series);
                model.Series.Add(scatterApp);
                model.Series.Add(scatterWarp);
            }

            if (Sessions.Count > 0 && Sessions[0].reprojectionStart.Count > 0)
            {
                LinearAxis linearAxisX = new LinearAxis();
                linearAxisX.Minimum = minValueX;
                linearAxisX.Maximum = maxValueX;
                linearAxisX.Title = "frame start during recording time s";
                linearAxisX.Position = AxisPosition.Bottom;

                LinearAxis linearAxisY = new LinearAxis();
                linearAxisY.Minimum = 0.0;
                linearAxisY.Maximum = maxValueY + minValueY;
                linearAxisY.Title = "reprojection time ms";
                linearAxisY.Position = AxisPosition.Left;

                model.Axes.Clear();
                model.Axes.Add(linearAxisX);
                model.Axes.Add(linearAxisY);
            }

            model.IsLegendVisible = true;
            model.LegendItemOrder = LegendItemOrder.Reverse;
            model.LegendPlacement = LegendPlacement.Inside;
            model.LegendPosition = LegendPosition.RightTop;
            model.LegendLineSpacing = 1.0;

            if (Sessions.Count == 1)
                model.Subtitle = sessions[0].Filename;

            Graph = model;
            reprojectionGraph = model;
            Type = GraphType.Reprojections;
        }

        private void UpdateFramesStatsMissed()
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

                // only add warp misses if it is a VR title
                if (Sessions[iSession].warpMissed.Count() > 0)
                {
                    missedWarpFrames.Items.Add(new ColumnItem(((double)Sessions[iSession].warpMissesCount
                    / Sessions[iSession].appMissed.Count()) * 100, iSession));
                }

                categoryAxis.Labels.Add(Sessions[iSession].filename);
            }

            missedAppFrames.LabelPlacement = LabelPlacement.Inside;
            missedAppFrames.LabelFormatString = "{0:0.00}%";
            missedWarpFrames.LabelPlacement = LabelPlacement.Outside;
            missedWarpFrames.LabelFormatString = "{0:0.00}%";
            missedWarpFrames.LabelMargin = 0.0;

            successFrames.TrackerFormatString = "{0}\n{1}: {2:.00}%";
            missedAppFrames.TrackerFormatString = "{0}\n{1}: {2:.00}%";
            missedWarpFrames.TrackerFormatString = "{0}\n{1}: {2:.00}%";

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

            if (Sessions.Count == 1)
                model.Subtitle = sessions[0].Filename;

            Graph = model;
            missedframesGraph = model;
            Type = GraphType.Misses;

            SubType = " Misses ";
        }

        private void UpdateFramesStatsNotStacked()
        {
            PlotModel model = new PlotModel();
            ColumnSeries series = new ColumnSeries();
            var categoryAxis = new CategoryAxis();
            categoryAxis.IsAxisVisible = false;
            LinearAxis linearAxis = new LinearAxis();
            linearAxis.AbsoluteMinimum = 0;
            linearAxis.MaximumPadding = 0.06;
            switch (metric)
            {
            case Metric.AvgFPS:
            {
                model.Title = "Average FPS";
                SubType = " Avg FPS ";
                for (var iSession = 0; iSession < Sessions.Count; iSession++)
                {
                    series.Items.Add(new ColumnItem(
                        Sessions[iSession].validAppFrames / Sessions[iSession].lastFrameTime, iSession));
                    categoryAxis.Labels.Add(Sessions[iSession].filename);
                }

                series.LabelPlacement = LabelPlacement.Middle;
                series.LabelFormatString = "{0:.00} FPS";
                series.TrackerFormatString = "{0}\n{1}: {2:.00} FPS";

                linearAxis.Title = "FPS";
                break;
            }
            case Metric.AvgFrameTimes:
            {
                SubType = " Avg frame times ";
                model.Title = "Average frame times";
                for (var iSession = 0; iSession < Sessions.Count; iSession++)
                {
                    series.Items.Add(new ColumnItem(
                        (Sessions[iSession].lastFrameTime * 1000.0) / Sessions[iSession].validAppFrames, iSession));
                    categoryAxis.Labels.Add(Sessions[iSession].filename);
                }

                series.LabelPlacement = LabelPlacement.Middle;
                series.LabelFormatString = "{0:.00}ms";
                series.TrackerFormatString = "{0}\n{1}: {2:.00}ms";
                linearAxis.Title = "ms";
                break;
            }
            case Metric.AvgReprojections:
            {
                model.Title = "Average reproj. times";
                SubType = " Avg reproj. times ";

                for (var iSession = 0; iSession < Sessions.Count; iSession++)
                {
                    series.Items.Add(new ColumnItem(
                        (Sessions[iSession].lastReprojectionTime * 1000.0) /
                        Sessions[iSession].validReproFrames, iSession));
                    categoryAxis.Labels.Add(Sessions[iSession].filename);
                }

                series.LabelPlacement = LabelPlacement.Middle;
                series.LabelFormatString = "{0:.00}ms";
                series.TrackerFormatString = "{0}\n{1}: {2:.00}ms";

                linearAxis.Title = "ms";
                break;
            }
            case Metric.NinetyninthPerc:
            {
                model.Title = "99th-percentile";
                SubType = " 99th-percentile ";
                for (var iSession = 0; iSession < Sessions.Count; iSession++)
                {
                    List<double> temp = Sessions[iSession].frameTimes;
                    temp.Sort();
                    var rank = (int)(0.99 * temp.Count());
                    series.Items.Add(new ColumnItem(temp[rank], iSession));
                    categoryAxis.Labels.Add(Sessions[iSession].filename);
                }

                series.LabelPlacement = LabelPlacement.Middle;
                series.LabelFormatString = "{0:.00}ms";
                series.TrackerFormatString = "{0}\n{1}: {2:.00}ms";
                linearAxis.Title = "ms";
                break;
            }
            }

            model.Series.Add(series);

            model.Axes.Clear();
            model.Axes.Add(categoryAxis);
            model.Axes.Add(linearAxis);

            model.IsLegendVisible = true;
            model.LegendPosition = LegendPosition.RightTop;
            model.LegendPlacement = LegendPlacement.Outside;
            model.LegendItemOrder = LegendItemOrder.Reverse;
            model.PlotAreaBorderThickness = new OxyThickness(1, 0, 0, 1);

            if (Sessions.Count == 1)
                model.Subtitle = sessions[0].Filename;

            Graph = model;
            missedframesGraph = model;
            Type = GraphType.Misses;
        }

        private void UpdateFramesStats()
        {
            NextOption = true;
            PrevOption = true;
            if (metric == Metric.MissedFrames)
            {
                UpdateFramesStatsMissed();
            }
            else 
            {
                UpdateFramesStatsNotStacked();
            }
            AdditionalOption = "Metric: ";
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
            NextOption = true;
            PrevOption = true;

            Type = GraphType.Misses;
            Graph = missedframesGraph;
            displayedIndex = -1;
            EnableFrameDetail = SessionSelected;
            AdditionalOption = "Metric: ";

            switch (metric)
            {
            case Metric.MissedFrames:
                {
                    SubType = " Misses ";
                    return;
                }
            case Metric.AvgFPS:
                {
                    SubType = " Avg FPS ";
                    return;
                }
            case Metric.AvgFrameTimes:
                {
                    SubType = " Avg frame times ";
                    return;
                }
            case Metric.AvgReprojections:
                {
                    SubType = " Avg reproj. times ";
                    return;
                }
            case Metric.NinetyninthPerc:
                {
                    SubType = " 99th-percentile ";
                    return;
                }
            }
        }

        public void SelectPreviousOption()
        {
            switch (Type)
            {
            case GraphType.FrameDetail:
            {
                if (frameRanges[displayedIndex].Item1 > 0)
                {
                    frameRanges[displayedIndex] = Tuple.Create(frameRanges[displayedIndex].Item1 - 400, frameRanges[displayedIndex].Item1 + 100);
                    savedframedetailIndex = -1;
                }

                ShowFrameEvents();
                return;
            }
            case GraphType.Misses:
            {
                --metric;
                if (metric < 0)
                {
                    metric = Metric.NinetyninthPerc;
                }
                UpdateFramesStats();
                return;
            }
            }
        }

        public void SelectNextOption()
        {
            switch (Type)
            {
            case GraphType.FrameDetail:
            {
                if ((sessions[displayedIndex].frameStart.Count() - 1) <= frameRanges[displayedIndex].Item2 + 500
                    && (sessions[displayedIndex].frameStart.Count() - 1) > frameRanges[displayedIndex].Item2)
                {
                    frameRanges[displayedIndex] = Tuple.Create(frameRanges[displayedIndex].Item1 + 400, sessions[displayedIndex].frameStart.Count() - 1);
                    savedframedetailIndex = -1;
                }
                else if ((sessions[displayedIndex].frameStart.Count() - 1) > frameRanges[displayedIndex].Item2)
                {
                    frameRanges[displayedIndex] = Tuple.Create(frameRanges[displayedIndex].Item1 + 400, frameRanges[displayedIndex].Item2 + 400);
                    savedframedetailIndex = -1;
                }

                ShowFrameEvents();
                return;
            }
            case GraphType.Misses:
            {
                metric = ++metric;
                if ((int)metric == maxMetricsNum)
                {
                    metric = Metric.MissedFrames;
                }
                UpdateFramesStats();
                return;
            }
            }
        }

        public void ShowFrameEvents()
        {
            // can't update if no session is selected, just keep current graph
            if (!SessionSelected)
            {
                if (displayedIndex == -1)
                {
                    return;
                }
                Graph = framedetailGraph;
                Type = GraphType.FrameDetail;
                savedframedetailIndex = displayedIndex;
                SubType = " " + frameRanges[displayedIndex].Item1 + " - " + frameRanges[displayedIndex].Item2 + " ";
                AdditionalOption = "Frames: ";
                NextOption = (frameRanges[displayedIndex].Item2 < (Sessions[displayedIndex].frameStart.Count() - 1));
                PrevOption = (frameRanges[displayedIndex].Item1 != 0);
                return;
            }

            // no need to recreate graph if current is still valid and in the correct range
            if (savedframedetailIndex == SelectedIndex)
            {
                Type = GraphType.FrameDetail;
                Graph = framedetailGraph;
                displayedIndex = savedframedetailIndex;
                EnableFrameDetail = false;
                SubType = " " + frameRanges[SelectedIndex].Item1 + " - " + frameRanges[SelectedIndex].Item2 + " ";
                AdditionalOption = "Frames: ";
                NextOption = (frameRanges[displayedIndex].Item2 < (Sessions[displayedIndex].frameStart.Count() - 1));
                PrevOption = (frameRanges[displayedIndex].Item1 != 0);
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

            // limit displayed frames to ~500 due to performance issues if too many frames/series are loaded into graph
            for (var i = frameRanges[SelectedIndex].Item1; i < Sessions[SelectedIndex].frameStart.Count() && i <= frameRanges[SelectedIndex].Item2; i++)
            {
                LineSeries series = new LineSeries();
                series.Mapping = handler;
                List<EventDataPoint> frame = new List<EventDataPoint>();

                if (Sessions[SelectedIndex].isVR)
                {
                    vSyncIndicators.Points.Add(new DataPoint(Sessions[SelectedIndex].vSync[i], 300));
                    if (Sessions[SelectedIndex].frameStart[i] != 0)
                        frame.Add(new EventDataPoint(Sessions[SelectedIndex].frameStart[i], lineRow + 106, "Start frame (App)"));
                    if (Sessions[SelectedIndex].frameEnd[i] != 0)
                        frame.Add(new EventDataPoint(Sessions[SelectedIndex].frameEnd[i], lineRow + 106, "End frame (App)"));
                    if (Sessions[SelectedIndex].reprojectionStart[i] != 0)
                        frame.Add(new EventDataPoint(Sessions[SelectedIndex].reprojectionStart[i], lineRow + 106, "Start Reprojection"));
                    if (Sessions[SelectedIndex].reprojectionEnd[i] != 0)
                        frame.Add(new EventDataPoint(Sessions[SelectedIndex].reprojectionEnd[i], lineRow + 106, "End Reprojection"));
                }
                else
                {
                    if (Sessions[SelectedIndex].frameStart[i] != 0)
                        frame.Add(new EventDataPoint(Sessions[SelectedIndex].frameStart[i], lineRow + 106, "Start frame"));
                    if (Sessions[SelectedIndex].frameEnd.Count() > i && Sessions[SelectedIndex].frameEnd[i] != 0)
                        frame.Add(new EventDataPoint(Sessions[SelectedIndex].frameEnd[i], lineRow + 106, "Render Complete"));
                    if (Sessions[SelectedIndex].reprojectionEnd.Count() > i && Sessions[SelectedIndex].reprojectionEnd[i] != 0)
                        frame.Add(new EventDataPoint(Sessions[SelectedIndex].reprojectionEnd[i], lineRow + 106, "Displayed"));
                }

                series.ItemsSource = frame;

                // at the beginning we jump to the first few frames, axis values depend on the specific session
                if (xAxisMinimum == 0)
                {
                    xAxisMinimum = Sessions[SelectedIndex].frameStart[i];
                    if (xAxisMinimum == 0)
                    {
                        xAxisMinimum = Sessions[SelectedIndex].reprojectionStart[i];
                    }
                }

                if (Sessions[SelectedIndex].reprojectionEnd.Count() > i)
                {
                    if (i <= frameRanges[SelectedIndex].Item1 + 3 && xAxisMaximum < Sessions[SelectedIndex].reprojectionEnd[i])
                        xAxisMaximum = Sessions[SelectedIndex].reprojectionEnd[i];
                } else
                {
                    // probably don't have reprojection end data here, just use frame start event timestamp
                    if (i <= frameRanges[SelectedIndex].Item1 + 3 && xAxisMaximum < Sessions[SelectedIndex].frameStart[i])
                        xAxisMaximum = Sessions[SelectedIndex].frameStart[i];
                }

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
                else if (Sessions[SelectedIndex].isVR && Sessions[SelectedIndex].warpMissed[i])
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
            xAxis.MaximumRange = 10.0;

            LinearAxis yAxis = new LinearAxis();
            yAxis.Minimum = 103;
            yAxis.Maximum = 110;
            yAxis.Position = AxisPosition.Left;
            yAxis.IsAxisVisible = false;

            yAxis.MinimumRange = 2.0;
            yAxis.MaximumRange = 10.0;

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

            NextOption = (frameRanges[displayedIndex].Item2 < (Sessions[SelectedIndex].frameStart.Count() - 1));
            PrevOption = (frameRanges[SelectedIndex].Item1 != 0);

            SubType = " " + frameRanges[SelectedIndex].Item1 + " - " + frameRanges[SelectedIndex].Item2 + " ";
            AdditionalOption = "Frames: ";
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

        public string SubType
        {
            get { return subType; }
            set
            {
                subType = value;
                this.NotifyPropertyChanged("SubType");
            }
        }

        public string AdditionalOption
        {
            get { return additionalOption; }
            set
            {
                additionalOption = value;
                this.NotifyPropertyChanged("AdditionalOption");
            }
        }

        public bool NextOption
        {
            get { return nextOption; }
            set
            {
                nextOption = value;
                this.NotifyPropertyChanged("NextOption");
            }
        }

        public bool PrevOption
        {
            get { return prevOption; }
            set
            {
                prevOption = value;
                this.NotifyPropertyChanged("PrevOption");
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
