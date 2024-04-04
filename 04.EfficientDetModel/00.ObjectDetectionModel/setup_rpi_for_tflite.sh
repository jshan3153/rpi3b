#파일 위치
https://gist.github.com/khanhlvg/bbeb5e4ccfca6cbcf18508a44f5964be

# Show your Raspberry Pi OS version.
cat /etc/os-release

# Update packages on your Raspberry Pi OS.
sudo apt-get update

# Check your Python version. You should have Python 3.7 or later.
python3 --version

# Install virtualenv and upgrade pip.
python3 -m pip install --user --upgrade pip
python3 -m pip install --user virtualenv

# Create a Python virtual environment for the TFLite samples (optional but strongly recommended)
python3 -m venv ~/tflite

# Run this command whenever you open a new Terminal window/tab to activate the environment.
source ~/tflite/bin/activate

# Clone the TensorFlow example repository with the TFLite Raspberry Pi samples.
git clone https://github.com/tensorflow/examples.git
cd examples/lite/examples/object_detection/raspberry_pi

# Install dependencies required by the sample
sh setup.sh

####
# If you see an error running the sample:
# ImportError: libcblas.so.3: cannot open shared object file: No such file or directory
# you can fix it by installing an OpenCV dependency that is missing on your Raspberry Pi.
sudo apt-get install libatlas-base-dev

####
#raise OSError('PortAudi library not found')
#OSError:PortAudio library not found
sudo apt-get install libportaudio2 
# Run the object detection sample
# **IMPORTANT**: If you SSH to the Pi, make sure that:
#  1. There is a display connected to the Pi.
#  2. Run `export DISPLAY=:0` before proceed to make the object_detection window appear on the display.
python detect.py

#deactivate virtual renvironment
deactivate

