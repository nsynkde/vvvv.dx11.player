//-----------------------------------------------------------------------------
// File: DXPlayback.cs
//-----------------------------------------------------------------------------
using System;
using System.Drawing;
using System.Windows.Forms;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Direct3D=Microsoft.DirectX.Direct3D;
using BluePlaybackNetLib;

namespace DXPlayback
{
	public class DXPlayback : Form
	{
		// Our global variables for this project
		Device device = null; // Our rendering device
		VertexBuffer vertexBuffer = null;
		PresentParameters presentParams = new PresentParameters();
        BluePlaybackNet BluePlayback = new BluePlaybackNet();
        Texture RenderTexture = null;
        Surface RenderSurface = null;
        Surface OffscreenSurface = null;
        RenderToSurface rts = null;
        const int ScreenWidth = 720;
        const int ScreenHeight = 576;

		bool pause = false;

		public DXPlayback()
		{
			// Set the initial size of our form
            this.ClientSize = new System.Drawing.Size(ScreenWidth, ScreenHeight);
			// And it's caption
			this.Text = "Bluefish Technologies - DXPlayback sample app";
		}

		public bool InitializeGraphics()
		{
			try
			{
				// Now let's setup our D3D stuff
				presentParams.Windowed=true;
				presentParams.SwapEffect = SwapEffect.Discard;
                presentParams.BackBufferWidth = ScreenWidth; 
                presentParams.BackBufferHeight = ScreenHeight;
				device = new Device(0, DeviceType.Reference, this, CreateFlags.SoftwareVertexProcessing, presentParams);
				device.DeviceReset += new System.EventHandler(this.OnResetDevice);
				this.OnCreateDevice(device, null);
				this.OnResetDevice(device, null);
				pause = false;
				return true;
			}
			catch (DirectXException)
            { 
                return false; 
            }
		}
		public void OnCreateDevice(object sender, EventArgs e)
		{
			Device dev = (Device)sender;
			// Now Create the VB
			vertexBuffer = new VertexBuffer(typeof(CustomVertex.PositionColored), 3, dev, 0, CustomVertex.PositionColored.Format, Pool.Default);
			vertexBuffer.Created += new System.EventHandler(this.OnCreateVertexBuffer);
			this.OnCreateVertexBuffer(vertexBuffer, null);
		}
		public void OnResetDevice(object sender, EventArgs e)
		{
			Device dev = (Device)sender;
			// Turn off culling, so we see the front and back of the triangle
			dev.RenderState.CullMode = Cull.None;
			// Turn off D3D lighting, since we are providing our own vertex colors
			dev.RenderState.Lighting = false;
		}
		public void OnCreateVertexBuffer(object sender, EventArgs e)
		{
			VertexBuffer vb = (VertexBuffer)sender;
			CustomVertex.PositionColored[] verts = (CustomVertex.PositionColored[])vb.Lock(0,0);
			verts[0].X=-1.0f; verts[0].Y=-1.0f; verts[0].Z=0.0f; verts[0].Color = System.Drawing.Color.Red.ToArgb();
			verts[1].X=1.0f; verts[1].Y=-1.0f ;verts[1].Z=0.0f; verts[1].Color = System.Drawing.Color.Blue.ToArgb();
			verts[2].X=0.0f; verts[2].Y=1.0f; verts[2].Z = 0.0f; verts[2].Color = System.Drawing.Color.Green.ToArgb();
			vb.Unlock();
		}

        public void InitBlueInterface()
        {
            rts = new RenderToSurface(device, ScreenWidth, ScreenHeight, Format.X8R8G8B8, true, DepthFormat.D16);
            RenderTexture = new Texture(device, ScreenWidth, ScreenHeight, 1, Usage.RenderTarget, Format.X8R8G8B8, Pool.Default);
            RenderSurface = RenderTexture.GetSurfaceLevel(0);
            OffscreenSurface = device.CreateOffscreenPlainSurface(ScreenWidth, ScreenHeight, Format.X8R8G8B8, Pool.Default);

            BluePlayback.BluePlaybackInterfaceCreate();
            BluePlayback.BluePlaybackInterfaceConfig(1, 0, 0, 6, 1, 0, 0, 0); //Dev 1, Output channel A, PAL, BGRA, FRAME_MODE, not used, not used, not used
        }

        public void RenderIntoSurface()
        {
            // Render to this surface
            Viewport view = new Viewport();
            view.Width = ScreenWidth;
            view.Height = ScreenHeight;
            view.MaxZ = 1.0f;

            rts.BeginScene(RenderSurface, view);

            device.Clear(ClearFlags.Target | ClearFlags.ZBuffer, Color.DarkBlue, 1.0f, 0);
            SetupMatrices();
            device.SetStreamSource(0, vertexBuffer, 0);
            device.VertexFormat = CustomVertex.PositionColored.Format;
            device.DrawPrimitives(PrimitiveType.TriangleList, 0, 1);

            rts.EndScene(Filter.None);

            //this is VERY slow!!!
            SurfaceLoader.FromSurface(OffscreenSurface, RenderSurface, Filter.None, 0);
            //this does not work
            //device.GetRenderTargetData(OffscreenSurface, RenderSurface);
            RenderBlueInterface(OffscreenSurface);
        }

        public void RenderBlueInterface(Surface sfc)
        {
            GraphicsStream GStream = sfc.LockRectangle(LockFlags.None);
            BluePlayback.BluePlaybackInterfaceRender(GStream.InternalData);
            sfc.UnlockRectangle();
        }

        public void ExitBlueInterface()
        {
            BluePlayback.BluePlaybackInterfaceDestroy();
        }

		private void Render()
		{
			if (device == null) 
				return;

			if (pause)
				return;

            RenderIntoSurface();
#if FALSE
            //Clear the backbuffer to a blue color 
			device.Clear(ClearFlags.Target, System.Drawing.Color.Blue, 1.0f, 0);
			//Begin the scene
			device.BeginScene();
			// Setup the world, view, and projection matrices
			SetupMatrices();
	
			device.SetStreamSource(0, vertexBuffer, 0);
			device.VertexFormat = CustomVertex.PositionColored.Format;
			device.DrawPrimitives(PrimitiveType.TriangleList, 0, 1);

            //using (Sprite s = new Sprite(device))
            //{
            //    s.Begin(SpriteFlags.None);
            //    s.Draw(RenderTexture, new Rectangle(0, 0, ScreenWidth, ScreenHeight), new Vector3(0, 0, 0), new Vector3(0, 0, 1.0f), Color.White);
            //    s.End();
            //}

			//End the scene
			device.EndScene();
            
#endif
			device.Present();
            
		}

		private void SetupMatrices()
		{
			int  iTime  = Environment.TickCount % 1000;
			float fAngle = iTime * (2.0f * (float)Math.PI) / 1000.0f;
			device.Transform.World = Matrix.RotationY( fAngle );
			device.Transform.View = Matrix.LookAtLH( new Vector3( 0.0f, 3.0f,-5.0f ), new Vector3( 0.0f, 0.0f, 0.0f ), new Vector3( 0.0f, 1.0f, 0.0f ) );
			device.Transform.Projection = Matrix.PerspectiveFovLH( (float)Math.PI / 4, 1.0f, 1.0f, 100.0f );
		}

		protected override void OnPaint(System.Windows.Forms.PaintEventArgs e)
		{
			this.Render(); // Render on painting
		}
		protected override void OnKeyPress(System.Windows.Forms.KeyPressEventArgs e)
		{
			if ((int)(byte)e.KeyChar == (int)System.Windows.Forms.Keys.Escape)
				this.Close(); // Esc was pressed
		}
        protected override void OnResize(System.EventArgs e)
        {
            pause = ((this.WindowState == FormWindowState.Minimized) || !this.Visible);
        }

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		static void Main() 
		{
            using (DXPlayback frm = new DXPlayback())
            {
                if (!frm.InitializeGraphics()) // Initialize Direct3D
                {
                    MessageBox.Show("Could not initialize Direct3D.  This tutorial will exit.");
                    return;
                }
                MessageBox.Show("InitBlueInterface");
                frm.InitBlueInterface();
                frm.Show();

                // While the form is still valid, render and process messages
                while(frm.Created)
                {
                    frm.Render();
                    Application.DoEvents();
                }
                frm.ExitBlueInterface();
            }
        }
	}
}
