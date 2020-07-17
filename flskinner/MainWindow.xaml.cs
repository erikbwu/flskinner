using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace flskinner
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();

            Bootstrap.Setup();

            int selectedIndex = -1;

            SkinsList.ItemsSource = Skin.skins;
            SkinsList.SelectedIndex = selectedIndex;
        }

        private void LaunchFL_Click(object sender, RoutedEventArgs e)
        {
            Core.inject(Config.current.flStudioPath + @"\FL64.exe");
        }

        private void OpenSkinsFolder_Click(object sender, RoutedEventArgs e)
        {
            Process.Start("explorer.exe", Skin.GetSkinsFolder());
        }

        private void SkinsList_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (e.AddedItems[0] != null)
            {
                Config.current.currentSkin = ((Skin)e.AddedItems[0]).fileName;
                Config.current.Save();
            }
        }
    }
}
