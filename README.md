# BadVXP - Bad Apple but for MediaTek phones (capable of running VXP files)

[![Watch a demo of it on YouTube](http://img.youtube.com/vi/3qnpoi0SPm8/0.jpg)](http://www.youtube.com/watch?v=3qnpoi0SPm8 "Playing Bad Apple on a... keypad phone? (BadVXP MRE app demo)")

# Notes

Currently, only 240x320 is supported and has been only tested on a MT6276 device (Allview M9 Join/Sagereal X241).

I do not have any MT625x or MT626x (have one, but can't launch VXPs: Maxcom MM35D) device with VXP support to test this with. If you do and find this functioning, consider DMing me on Discord (raul0028.1, please FRQ as I have DMs disabled to non-friends) or pinging me on the ROMPhonix server.

# Requirements

1. Python (for the encoder)

- 1.1 PyLZSS0 from here: https://github.com/HoodedTissue/pylzss0.git (note: when installing on Windows, make sure you run as admin)

2. FFMPEG

3. A Bad Apple!! video

4. ARM RVCT 3.1 (for real device compilation)

5. Visual Studio 2008 (for MoDIS compilation)

6. MediaTek's MRE SDK

# Setup

1. Install PyLZSS0

```
cd <project>\pylzss0
python setup.py install
```

2. Get your Bad Apple!! video from a YT download, SWF converter, whatever.

3. Do the following things with FFMPEG:

```
ffmpeg -i <video> -vf "scale=320:240" <project>\sources\scaled.mp4
ffmpeg -i <project>\sources\scaled.mp4 -vf "transpose=1" <project>\sources\rotated.mp4
ffmpeg -i <project>\sources\rotated.mp4 <project>\sources\%04d.png
```

4. Prepare the frames to be in 1bpp

```
<project>\sources\tomono.py <project>\frames <project>\frames_1bpp
```

5. Encode to a ani file

```
<project>\sources\encode.py <project>\frames_1bpp <project>\ani.ani -p <processes for multithread, leave it as 1 if you don't want to>
```

6. Build the project

7. For testing, place the ani.ani file onto <SD CARD>:\BadApple folder. Then put the Default.vxp file onto your SD card.

8. Run VXP and enjoy!