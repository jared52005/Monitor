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
        FileSystemWatcher _watcher;
        public List<int> IgnoredIds { get; set; }
        public List<int> Iso15765Ids { get; set; }
        public CanIds(string path)
        {
            Iso15765Ids = new List<int>(new int[]{ 0x700, 0x7E0, 0x7E8, 0x7E1, 0x7E9 }); //Default setup for ISO15765

            //Check if path is valid
            if (string.IsNullOrEmpty(path))
            {
                return;
            }

            if (!File.Exists(path))
            {
                Console.WriteLine($"File {path} does not exist");
                return;
            }
            ParseXml(path);

            var directory = Path.GetDirectoryName(path);
            var filename = Path.GetFileName(path);
            _watcher = new FileSystemWatcher(directory, filename);
            _watcher.NotifyFilter = NotifyFilters.LastWrite;
            _watcher.Changed += Watcher_Changed;
            _watcher.EnableRaisingEvents = true;

        }

        DateTime _lastWrite;
        private void Watcher_Changed(object sender, FileSystemEventArgs e)
        {
            //Bug - FileSystemEvent is fired twice on one change.
            DateTime lastWriteTime = File.GetLastWriteTime(e.FullPath);
            if (lastWriteTime != _lastWrite)
            {
                _lastWrite = lastWriteTime;
                System.Threading.Thread.Sleep(500); //Give 3rd party application some time to finish writing.
                                                    //Yes there should be some smarter check. This is lazy, but usually works.
                Console.WriteLine("CAN ID file changed, update...");
                ParseXml(e.FullPath);
            }
        }

        private void ParseXml(string path)
        {
            try
            {
                List<int> ignored = new List<int>();
                List<int> iso15765 = new List<int>();
                XElement root = XElement.Load(path);
                var xcanIds = root.Elements("canid");
                foreach (XElement xcanId in xcanIds)
                {
                    if (xcanId.Attribute("action") == null)
                    {
                        throw new Exception($"missing attribute action at {xcanId}");
                    }

                    int id;
                    switch (xcanId.Attribute("action").Value)
                    {
                        case "ignore":
                            id = Convert.ToInt32(xcanId.Value, 16);
                            ignored.Add(id);
                            break;
                        case "iso15765":
                            id = Convert.ToInt32(xcanId.Value, 16);
                            iso15765.Add(id);
                            break;
                        default:
                            throw new Exception($"Invalid action attribute at {xcanId}");
                    }
                }

                if (ignored.Count != 0)
                {
                    Console.WriteLine("Ignoring following IDs:");
                    foreach (int id in ignored)
                    {
                        Console.Write($"{id:X} ");
                    }
                    Console.WriteLine();
                }
                if (iso15765.Count != 0)
                {
                    Console.WriteLine("Expecting ISO15765 on following IDs:");
                    foreach (int id in iso15765)
                    {
                        Console.Write($"{id:X} ");
                    }
                    Console.WriteLine();
                }
                IgnoredIds = ignored;
                Iso15765Ids = iso15765;
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

        public bool IsIso15765(CanMessage msg)
        {
            if (Iso15765Ids == null)
            {
                return false;
            }

            var ids = Iso15765Ids;
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
            _watcher.Dispose();
        }
    }
}
