устанавливаем anydesk.
в терминале:
sudo -s
wget -qO - https://keys.anydesk.com/repos/DEB-GPG-KEY | apt-key add -
echo "deb http://deb.anydesk.com/ all main" > /etc/apt/sources.list.d/anydesk-stable.list
apt-get update
apt-get install anydesk


устанавливаем glade-gtk2 на Stretch или другой дистрибутив:

To enable the linuxcnc glade-gtk2 widget's in Ubuntu / Mint / Kali, etc. you have to install a little bit more file's :
http://www.linuxcnc.org/dists/wheezy/base/

for 32 bit :
http://www.linuxcnc.org/dists/wheezy/base/binary-i386/libgladeui-1-11_3.8.0-0ubuntu6_i386.deb
http://www.linuxcnc.org/dists/wheezy/base/binary-i386/glade-gtk2_3.8.0-0ubuntu6_i386.deb
http://www.linuxcnc.org/dists/wheezy/base/binary-i386/glade-gnome_3.8.0-0ubuntu6_i386.deb

for 64 bit :
http://www.linuxcnc.org/dists/wheezy/base/binary-amd64/libgladeui-1-11_3.8.0-0ubuntu6_amd64.deb
http://www.linuxcnc.org/dists/wheezy/base/binary-amd64/glade-gtk2_3.8.0-0ubuntu6_amd64.deb
http://www.linuxcnc.org/dists/wheezy/base/binary-amd64/glade-gnome_3.8.0-0ubuntu6_amd64.deb

First maybe install glade and geanny :
sudo apt-get install glade
sudo apt-get install geanny
Install the files in terminal like :

sudo dpkg -i libgladeui-1-11_3.8.0-0ubuntu6_amd64.deb
sudo dpkg -i glade-gtk2_3.8.0-0ubuntu6_amd64.deb
sudo dpkg -i glade-gnome_3.8.0-0ubuntu6_amd64.deb

Sometimes the terminal say's you have to fix broken packages, this is only for glade installation :

type : sudo apt --fix-broken install

