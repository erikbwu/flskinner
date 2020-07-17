using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace flskinner
{
    public class Skin
    {
        public string name { get; set; }
        public string author { get; set; }
        public string fileName;

        public static List<Skin> skins;

        public static string GetSkinsFolder()
        {
            var appDataPath = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
            return string.Format(@"{0}\flskinner\skins\", appDataPath);
        }
    }
}
