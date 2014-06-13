using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using BluePlaybackNetLib;

/*
 *
MEM_FMT_ARGB
MEM_FMT_BV10
MEM_FMT_BV8
MEM_FMT_YUVS
MEM_FMT_V210
MEM_FMT_RGBA
MEM_FMT_CINEON_LITTLE_ENDIAN
MEM_FMT_ARGB_PC
MEM_FMT_BGRA
MEM_FMT_CINEON
MEM_FMT_2VUY
MEM_FMT_BGR
MEM_FMT_BGR_16_16_16
MEM_FMT_BGRA_16_16_16_16
MEM_FMT_VUYA_4444
MEM_FMT_V216
MEM_FMT_Y210
MEM_FMT_Y216
MEM_FMT_RGB
MEM_FMT_YUV_ALPHA
MEM_FMT_INVALID
 */
namespace VVVV.Nodes.Bluefish
{
	class ModeRegister
	{
		public class ModeIndex
		{
			public ModeIndex(string Index)
			{
				this.Index = Index;
			}

			public string Index;
		}

        HashSet<BlueFish_h.EMemoryFormat> SupportedFormats = new HashSet<BlueFish_h.EMemoryFormat>();

		public class Mode
		{
            public int DisplayModeHandle; //IDeckLinkDisplayMode
            public BlueFish_h.EMemoryFormat PixelFormat;
            public int Flags; //_BMDVideoOutputFlags
			public int Width;
			public int Height;
			public double FrameRate;
			public int PixelGroupSizeOfSet { get; private set; }
			public int PixelGroupSizeInMemory { get; private set; }
			public int CompressedWidth
			{
				get
				{
					return Width * this.PixelGroupSizeInMemory / this.PixelGroupSizeOfSet / 4;
				}
			}

			public void CalcPixelBoundaries()
			{
				switch (this.PixelFormat)
				{
                    case BlueFish_h.EMemoryFormat.MEM_FMT_ARGB:
                    case BlueFish_h.EMemoryFormat.MEM_FMT_RGBA:
                    case BlueFish_h.EMemoryFormat.MEM_FMT_ARGB_PC:
                    //case BlueFish_h.EMemoryFormat.MEM_FMT_BGRA:
                        PixelGroupSizeOfSet = 1;
						PixelGroupSizeInMemory = 4;
                        break;

                    case BlueFish_h.EMemoryFormat.MEM_FMT_BV10:
                        break;

                    case BlueFish_h.EMemoryFormat.MEM_FMT_BV8:
                    //case BlueFish_h.EMemoryFormat.MEM_FMT_YUVS:
                        PixelGroupSizeOfSet = 2;
						PixelGroupSizeInMemory = 4;
                        break;

                    case BlueFish_h.EMemoryFormat.MEM_FMT_V210:
                        break;
                   
                    case BlueFish_h.EMemoryFormat.MEM_FMT_CINEON_LITTLE_ENDIAN:
                        break;
                    
                    case BlueFish_h.EMemoryFormat.MEM_FMT_CINEON:
                        break;

                    case BlueFish_h.EMemoryFormat.MEM_FMT_2VUY:
                        break;

                    case BlueFish_h.EMemoryFormat.MEM_FMT_BGR:
                        break;

                    case BlueFish_h.EMemoryFormat.MEM_FMT_BGR_16_16_16:
                        break;

                    case BlueFish_h.EMemoryFormat.MEM_FMT_BGRA_16_16_16_16:
                        break;

                    case BlueFish_h.EMemoryFormat.MEM_FMT_VUYA_4444:
                        break;

                    case BlueFish_h.EMemoryFormat.MEM_FMT_V216:
                        break;

                    case BlueFish_h.EMemoryFormat.MEM_FMT_Y210:
                        break;

                    case BlueFish_h.EMemoryFormat.MEM_FMT_Y216:
                        break;

                    case BlueFish_h.EMemoryFormat.MEM_FMT_RGB:
                        break;

                    case BlueFish_h.EMemoryFormat.MEM_FMT_YUV_ALPHA:
                        break;

                    case BlueFish_h.EMemoryFormat.MEM_FMT_INVALID:
                        break;


#if false
                    case _BMDPixelFormat.bmdFormat8BitYUV:
                        PixelGroupSizeOfSet = 2;
                        PixelGroupSizeInMemory = 4;
                        break;

                    case _BMDPixelFormat.bmdFormat10BitYUV:
                        PixelGroupSizeOfSet = 6;
                        PixelGroupSizeInMemory = 16;
                        break;

                    case _BMDPixelFormat.bmdFormat8BitBGRA:
                    case _BMDPixelFormat.bmdFormat8BitRGBA:
                    case _BMDPixelFormat.bmdFormat8BitARGB:
                        PixelGroupSizeOfSet = 1;
                        PixelGroupSizeInMemory = 4;
                        break;

                    case _BMDPixelFormat.bmdFormat10BitRGB:
                        PixelGroupSizeOfSet = 1;
                        PixelGroupSizeInMemory = 4;
                        break;

                    case _BMDPixelFormat.bmdFormat12BitYUV:
                        PixelGroupSizeOfSet = 4;
                        PixelGroupSizeInMemory = 6;
                        throw (new Exception("12bit modes are undocumented"));
                        break; 
#endif
				}
			}
		}

		public static ModeRegister Singleton = new ModeRegister();

		Dictionary<string, Mode> FModes = new Dictionary<string, Mode>();
		public Dictionary<string, Mode> Modes
		{
			get
			{
				return this.FModes;
			}
		}

		public ModeRegister()
		{
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_ARGB);
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_BV10);
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_BV8);
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_YUVS);
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_V210);
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_RGBA);
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_CINEON_LITTLE_ENDIAN);
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_ARGB_PC);
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_BGRA);
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_CINEON);
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_2VUY);
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_BGR);
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_BGR_16_16_16);
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_BGRA_16_16_16_16);
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_VUYA_4444);
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_V216);
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_Y210);
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_Y216);
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_RGB);
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_YUV_ALPHA);
            SupportedFormats.Add(BlueFish_h.EMemoryFormat.MEM_FMT_INVALID);
        
		}

		public void Refresh(int flags)
		{
			WorkerThread.Singleton.PerformBlocking(() =>
			{
				foreach (var mode in FModes.Values)
					Marshal.ReleaseComObject(mode.DisplayModeHandle);
				FModes.Clear();

				DeviceRegister.Singleton.Refresh();
				foreach (var device in DeviceRegister.Singleton.Devices)
				{
                    var output = (BluePlaybackNet)device.BluePlayback;
					if (output == null)
						throw (new Exception("Device has no outputs"));

					//IDeckLinkDisplayModeIterator modeIterator;
					//output.GetDisplayModeIterator(out modeIterator);

                    
                    /*
					while (true)
					{
						IDeckLinkDisplayMode mode;

						modeIterator.Next(out mode);
						if (mode == null)
							break;

                        foreach (BlueFish_h.EMemoryFormat pixelFormat in Enum.GetValues(typeof(BlueFish_h.EMemoryFormat)))
						{
							if (true || SupportedFormats.Contains(pixelFormat))
							{
								try
								{
									_BMDDisplayModeSupport support;
									IDeckLinkDisplayMode displayMode;
									output.DoesSupportVideoMode(mode.GetDisplayMode(), pixelFormat, flags, out support, out displayMode);

									string modeString;
									mode.GetName(out modeString);
									int stripLength = "bmdFormat".Length;
									string pixelFormatString = pixelFormat.ToString();
									pixelFormatString = pixelFormatString.Substring(stripLength, pixelFormatString.Length - stripLength);

									modeString += " [" + pixelFormatString + "]";

									long duration, timescale;
									mode.GetFrameRate(out duration, out timescale);

									Mode inserter = new Mode()
									{
										DisplayModeHandle = mode,
										Flags = flags,
										PixelFormat = pixelFormat,
										Width = mode.GetWidth(),
										Height = mode.GetHeight(),
										FrameRate = (double)timescale / (double)duration
									};
									inserter.CalcPixelBoundaries();

									if (support == _BMDDisplayModeSupport.bmdDisplayModeSupported)
									{
										FModes.Add(modeString, inserter);
									}
									else if (support == _BMDDisplayModeSupport.bmdDisplayModeSupportedWithConversion)
									{
										modeString += " converted";
										FModes.Add(modeString, inserter);
									}
								}
								catch
								{
								}
							}
						} 
                                          
					}

                    */
				}
			});
		}

		public string[] EnumStrings
		{
			get
			{
				return FModes.Keys.ToArray();
			}
		}
	}
}
