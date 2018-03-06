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

using Microsoft.Win32;
using System.Windows;
using System.Windows.Input;

namespace Frontend
{
    /// <summary>
    /// Interaction logic for VisualizationWindow.xaml
    /// </summary>
    public partial class VisualizationWindow : Window
    {
        PlotData plot = new PlotData();
        
        public VisualizationWindow(string csvFile)
        {
            InitializeComponent();
            PlotData data = new PlotData();
            this.DataContext = plot;
            plot.LoadData(csvFile);
        }

        private void FrameTimeButton_Click(object sender, RoutedEventArgs e)
        {
            plot.ShowFrameTimes();
        }

        private void MissedFramesButton_Click(object sender, RoutedEventArgs e)
        {
            plot.ShowMissedFramesTimes();
        }

        private void ReprojectionButton_Click(object sender, RoutedEventArgs e)
        {
            plot.ShowReprojectionTimes();
        }

        private void AddSessionButton_Click(object sender, RoutedEventArgs e)
        {
            OpenFileDialog fileDialog = new OpenFileDialog();
            fileDialog.Filter = "CSV|*.csv";

            bool? result = fileDialog.ShowDialog();
            if (result.HasValue && (bool)result)
            {
                plot.AddSessionPath = fileDialog.FileName;
            }
        }

        private void LoadSessionButton_Click(object sender, RoutedEventArgs e)
        {
            plot.LoadData(plot.AddSessionPath);
            listboxTest.Items.Refresh();
        }

        private void RemoveSessionButton_Click(object sender, RoutedEventArgs e)
        {
            plot.UnloadData();
            listboxTest.Items.Refresh();
        }

        private void SaveGraphButton_Click(object sender, RoutedEventArgs e)
        {
            SaveFileDialog fileDialog = new SaveFileDialog();
            fileDialog.Filter = "SVG|*.svg";

            bool? result = fileDialog.ShowDialog();
            if (result.HasValue && (bool)result)
            {
                plot.SaveSvg(fileDialog.FileName);
            }
        }

        private void MinimizeButton_Click(object sender, RoutedEventArgs e)
        {
            this.WindowState = System.Windows.WindowState.Minimized;
        }

        private void CloseButton_Click(object sender, RoutedEventArgs e)
        {
            this.Close();
        }

        private void Border_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            DragMove();
        }

        private void showSingleFrameButton_Click(object sender, RoutedEventArgs e)
        {
            plot.Type = GraphType.FrameDetail;
            plot.ShowFrameEvents();
        }

        private void listboxTest_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            plot.UpdateSelectionIndex();
        }

        private void frameSectionBackwards_Click(object sender, RoutedEventArgs e)
        {
            plot.JumpBackFrames();
        }

        private void frameSectionForwards_Click(object sender, RoutedEventArgs e)
        {
            plot.JumpForwardFrames();
        }
    }
}
