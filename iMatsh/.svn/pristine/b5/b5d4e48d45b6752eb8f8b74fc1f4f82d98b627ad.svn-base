This is a test that is designed to be close to your use case; i.e. features are single vectors extending over 1s segments.
It uses 1s long FFTs and 1s hop. See extractAllWav.sh to see how the features were extracted.

# Step 1: make sure you have the latest iMatsh source (I last updated it on 8th December 2011 to seed the random number generator with the system time).

cd into your brainspotter directory (with the iMatsh source code)
svn update

#Step 2: Download test files (uses 8s amen break audio split into 1s chunks)

wget http://aum.dartmouth.edu/~mcasey/imatsh_test_amen.zip
unzip imatsh_test_amen.zip
cd imatsh_test_amen

#run the test
./run_test.sh

#dump the match output values
cat *.imatsh.txt

#You should see the output below, ordered as:
# dist, exp(-beta * dist), ampl, qpos, dbpos, filename
#
# 0.000000 1.000000 1.000000 0 0 amen00.wav
# -0.000000 1.000000 1.000000 0 0 amen01.wav
# 0.000000 1.000000 1.000000 0 0 amen02.wav
# 0.000000 1.000000 1.000000 0 0 amen03.wav
# 0.000000 1.000000 1.000000 0 0 amen04.wav
# -0.000000 1.000000 1.000000 0 0 amen05.wav
# -0.000000 1.000000 1.000000 0 0 amen06.wav
# 0.000000 1.000000 1.000000 0 0 amen07.wav
# 0.032592 1.000000 0.381627 0 0 amen00.wav
