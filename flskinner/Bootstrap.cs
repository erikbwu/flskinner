using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using Microsoft.Win32;
using Microsoft.WindowsAPICodePack.Dialogs;
using Newtonsoft.Json;

namespace flskinner
{
    class Bootstrap
    {
        public static void Setup()
        {
            var assembly = Assembly.GetExecutingAssembly();

            var appDataPath = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);

            var folderPath = string.Format(@"{0}\flskinner\", appDataPath);
            var skinsFolderPath = string.Format(@"{0}\flskinner\skins\", appDataPath);

            System.IO.Directory.CreateDirectory(folderPath);

            var mainConfigPath = folderPath + "flskinner.json";
            if (!File.Exists(mainConfigPath))
            {
                var stream = assembly.GetManifestResourceStream("flskinner.Default.flskinner.json");

                using (StreamReader reader = new StreamReader(stream))
                {
                    File.WriteAllText(mainConfigPath, reader.ReadToEnd());
                }
            }

            if (!Directory.Exists(skinsFolderPath))
            {
                Directory.CreateDirectory(skinsFolderPath);

                var prefix = "flskinner.Default.skins.";

                var resourceNames = Assembly.GetExecutingAssembly()
                    .GetManifestResourceNames()
                    .Where(name => name.StartsWith(prefix));

                foreach (var name in resourceNames) {
                    var stream = assembly.GetManifestResourceStream(name);

                    using (StreamReader reader = new StreamReader(stream))
                    {
                        var path = string.Format("{0}{1}", skinsFolderPath, name.Replace(prefix, ""));
                        File.WriteAllText(path, reader.ReadToEnd());
                    }
                }
            }

            try
            {
                Config.current = JsonConvert.DeserializeObject<Config>(File.ReadAllText(mainConfigPath));
            }
            catch (Exception e)
            {
                StringBuilder sb = new StringBuilder();
                sb.AppendLine("An exception occured while loading the configuration file (flskinner.json in %appdata%/flskinner)");
                sb.Append(e.Message);
                MessageBox.Show(sb.ToString());
                System.Environment.Exit(1);
            }

            // some users have it in the 64 bit program files :shrug:
            if (!File.Exists(string.Format(@"{0}\FL64.exe", Config.current.flStudioPath)))
            {
                var x64Path = @"C:\Program Files\Image-Line\FL Studio 20\FL64.exe";
                if (File.Exists(x64Path))
                {
                    Config.current.flStudioPath = x64Path;
                    Config.current.Save();
                }
            }

            if (!File.Exists(string.Format(@"{0}\FL64.exe", Config.current.flStudioPath)))
            {
                CommonOpenFileDialog dialog = new CommonOpenFileDialog();
                dialog.InitialDirectory = "C:\\";
                dialog.IsFolderPicker = true;
                dialog.Title = "Please select your FL studio folder (contains FL64.exe)";

                var res = dialog.ShowDialog();
                if (res == CommonFileDialogResult.Ok)
                {
                    var path = dialog.FileName + @"\FL64.exe";
                    if (File.Exists(path))
                    {
                        Config.current.flStudioPath = dialog.FileName;
                        Config.current.Save();
                    } else
                    {
                        MessageBox.Show("That folder does not contain FL studio!");

                        Setup();
                        return;
                    }
                } 
                else if (res == CommonFileDialogResult.Cancel)
                {
                    System.Environment.Exit(1);
                }
            }

            Skin.skins = new List<Skin>();

            foreach (var skinPath in Directory.GetFiles(skinsFolderPath))
            {
                try
                {
                    var skin = JsonConvert.DeserializeObject<Skin>(File.ReadAllText(skinPath));
                    skin.fileName = Path.GetFileName(skinPath);
                    Skin.skins.Add(skin);
                }
                catch (Exception e)
                {
                    StringBuilder sb = new StringBuilder();
                    sb.Append("An exception occured while loading the skin file ");
                    sb.AppendLine("'" + Path.GetFileName(skinPath) + "'");
                    sb.Append(e.Message);
                    MessageBox.Show(sb.ToString());
                    System.Environment.Exit(1);
                }
            }
        }
    }
}
