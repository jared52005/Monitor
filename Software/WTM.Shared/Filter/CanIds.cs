using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Linq;
using System.IO;

namespace WTM.Filter
{
    public class CanIds : IDisposable
    {
        string _path;
        public List<int> IgnoredIds { get; set; }
        public CanIds(string path)
        {
            if(string.IsNullOrEmpty(path))
            {
                return;
            }

            if (!File.Exists(path))
            {
                Console.WriteLine($"File {path} does not exist");
                return;
            }
            _path = path;
            ParseXml(path);
        }

        private void ParseXml(string path)
        {
            try
            {
                List<int> ignored = new List<int>();
                XElement root = XElement.Load(path);
                var xcanIds = root.Elements("canid");
                foreach (XElement xcanId in xcanIds)
                {
                    if (xcanId.Attribute("action") == null)
                    {
                        throw new Exception($"missing attribute action at {xcanId}");
                    }

                    switch (xcanId.Attribute("action").Value)
                    {
                        case "ignore":
                            int id = Convert.ToInt32(xcanId.Value, 16);
                            ignored.Add(id);
                            break;
                        default:
                            throw new Exception($"Invalid action attribute at {xcanId}");
                    }
                }

                Console.WriteLine("Ignoring following IDs:");
                foreach(int id in ignored)
                {
                    Console.Write($"{id:X} ");
                }
                Console.WriteLine();
                IgnoredIds = ignored;
            }
            catch(Exception ex)
            {
                Console.WriteLine($"XML Parsing Error: {ex.Message}");
            }
        }

        public bool Ignore(CanMessage msg)
        {
            if (IgnoredIds == null)
            {
                return false;
            }

            var ids = IgnoredIds;
            foreach (int id in ids)
            {
                if (msg.Id == id)
                {
                    return true;
                }
            }
            return false;
        }

        public void Dispose()
        {
            
        }
    }
}
