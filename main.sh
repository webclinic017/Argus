rm -rf build/ && 
mkdir build && 
cd build &&

cmake ../ && 
make && 
cd Argus && 
sudo python3 setup.py install &&
sudo pip3 install .