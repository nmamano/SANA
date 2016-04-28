/* 
 * Copyright (C) 2016 Wen, Chifeng <https://sourceforge.net/u/daviesx/profile/>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
package research;

import java.awt.Container;
import javax.swing.JDialog;
import javax.swing.JFrame;

/**
 *
 * @author davis
 */
public abstract class CustomDialog extends JFrame {
        
        public abstract void run();

        public abstract void set_data(Object data);

        public abstract Object get_data();

        public abstract String get_name();
        
        public Container get_container() {
                return this;
        }
        
        public void set_container(Container cont) {
                super.setContentPane(cont);
        }
}
