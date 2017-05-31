using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace DpsViewer {
	public partial class Info : Form {
		public Info() {
			InitializeComponent();
		}
		
		private void Info_Resize(object sender, EventArgs e) {
			if (FormWindowState.Minimized == this.WindowState) {
				notifyIcon1.Visible = true;
				this.Hide();
			} else if (FormWindowState.Normal == this.WindowState) {
				notifyIcon1.Visible = false;
			}
		}

		private void Info_FormClosed(object sender, FormClosedEventArgs e) {
			Program.quit();
		}

		private void notifyIcon1_Click(object sender, EventArgs e) {
			this.Show();
			this.WindowState = FormWindowState.Normal;
		}
	}
}
