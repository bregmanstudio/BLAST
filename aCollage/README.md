aCollage - audio feature-based collage and blend processor
==========================================================

Usage: aCollage target.features target.powers sourceFeatureList.txt sourcePowerList.txt MediaList.txt shingleSize hopSize 
[queueSize loFeature hiFeature beta numHits frameSizeInSamples frameHopInSamples randomMatch]


Matrices are stored in binary files. The format is architecture independent:
values are stored as binary IEEE little-endien float row major


target.features  - file containing matrix of features for target, one row per observation

target.powers    - file containing vector of powers for target, one value per observation

targetFilename.wav - media file of target (for blending with matched audio)

sourceFeatureList.txt  - list of filenames for database source feature matrices "

sourcePowerList.txt    - list of filenames for database source power vectors "

MediaList.txt - list of filenames for source media (.wav format)

shingleSize   - number of feature vectors to stack for similarity search

hopSize       - number of observation time-points to skip per query

[queueSize=0] - queue-size for matching without replacement

[loFeature=0] - start index of first element in feature vector

[hiFeature=10]- end index (inclusive) of last element in feature vector

[beta=2.0]    - mixing parameter, exp(-beta * dist)

[numHits=3]   = number of nearest neighbours to retrieve in similarity search

[frameSizeInSamples] - feature frame size (length) in samples [1024]

[frameHopInSamples] - feature hop length in samples [1024]

[randomMatch] - baseline generate random pos matches, for testing


Copyright (C) January 2010, Michael A. Casey, All Rights Reserved

Code adapted from public-domain SoundSpotter project by Michael A. Casey

Additional features:

	   K nearest neighbor search

	   Retrieved audio blending via probabilistic weighting

	   Off-line version for simplicity	   
	   

