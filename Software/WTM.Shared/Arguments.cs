using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Security.AccessControl;
using System.Text;
using System.Threading.Tasks;

namespace WTM.Shared
{
    public enum ArgumentTypes
    {
        CanIdsFile,
        ComPort,
        Baudrate,
    }

    public static class Arguments
    {
        public static Dictionary<ArgumentTypes, object> Parse(string[] args)
        {
            try
            {
                Dictionary<ArgumentTypes, object> pargs = new Dictionary<ArgumentTypes, object>();
                for (int i = 0; i < args.Length; i++)
                {
                    switch (args[i].ToLower())
                    {
                        case "-c":
                        case "-com":
                            pargs.Add(ArgumentTypes.ComPort, args[i + 1]);
                            break;
                        case "-i":
                        case "-canid":
                            string canidFilePath = args[i + 1];
                            if (File.Exists(canidFilePath))
                            {
                                pargs.Add(ArgumentTypes.CanIdsFile, canidFilePath);
                            }
                            else
                            {
                                throw new Exception($"Invalid path to CanIDs file: {canidFilePath}");
                            }
                            break;
                        case "-b":
                        case "-baudrate":
                            int baudarte = Convert.ToInt32(args[i + 1]);
                            pargs.Add(ArgumentTypes.Baudrate, baudarte);
                            break;
                        default:
                            break;
                    }
                }

                return pargs;
            }
            catch(Exception ex)
            {
                Console.WriteLine("Failed to parse aguments: " + ex.Message);
                return null;
            }
        }
    }
}
