vCollage - video feature-driven collage and blend processor
===========================================================
Michael A. Casey - Bregman Media Labs, Dartmouth College


Usage: vCollage movieListFile matshupFile [audioFps(46.875000) hopSize(8) motionThresh(20) decimationFactor(4) sizeX(1280) sizeY(720) videoFps(25.000000) display(0) verbosity(2)]


movieListFile - text file of move files, one per line (including paths)

matshupFile - output similarity map from aCollage or ADA 

[OPTIONS]

audioFps - audio frame rate, fps [46.875000]

hopSize - frames hopped per query, same as matshupFile [8]

motionThresh - threshold for flicker/motion extraction [20]

decimationFactor - downsampling factor for flicker/motion [4]

sizeX - output video width [1280]

sizeY - output video height [720]

videoFps - output video frame rate [25.000000]

display - whether to display output while processing [0]

verbosity - how much do you want to know [2]

