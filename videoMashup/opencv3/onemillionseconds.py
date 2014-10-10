# nn - Reduce N-nearest neighbors per query point from batch imatsh files
import scikits.audiolab as skaud
from numpy import *
import numpy as np
import numpy.lib.recfunctions as rfn
import glob, os, sys
import action.suite as acts
import pickle, json, pdb

class NNMapReduce(object):
    def __init__(self, SR=48000., FH=1024., shingle_hop=8, in_prefix='batch', in_postfix='??.imatsh.txt'):
        """
        Post-process (REDUCE) nearest neighbor search batch (MAP) files
        inputs:
            SR - audio sample rate of target media [48000.]
            FH - feature (FFT) frame hop (in samples) of target media [1024.]
            shingle_hop- shingle hop (in FFT frames) [8]
            in_prefix - file stem name of batch files ['batch']
            in_postfix - file postfix expression ['??.imatsh.txt']
        output:
            self.fList - sorted list of MAP batch files
            self.nnRaw - list of lists of raw (MAP) nearest neighbor results
        """
        self.SR = SR # audio sample rate
        self.FH = FH  # FFT frame hop
        self.FR = SR / FH # FFT frame rate
        self.shingle_hop = shingle_hop # Shingle hop in FFT frames
        self.in_prefix='batch'
        self.fields=[('dist','<f4'),('prob','<f4'),('amp','<f4'),('qpos','<i4'),('spos','<i4'),('idx','<i4')]
        self.fields_str = '%f %f %f %i %i %i'        
        self.in_postfix = in_postfix
        self.fList = glob.glob(in_prefix+in_postfix)
        self.fList.sort()
        if len(self.fList)==0:
            print "Warning: Didn't find batch files using dir_expr:", in_prefix+in_postfix
        else:
            print "Loading",len(self.fList),"batch files, dir_expr =", in_prefix+in_postfix
        self.nnRaw  = [np.loadtxt(f,dtype=self.fields) for f in self.fList]
        
    def reduce_nearest_neighbor_batches(self, K=5, crop_pos=300, end_pos=3600, out_fname = 'movie.imatsh.txt'):
        """
        REDUCE nearest neighbor results from batch files
        Assumes that MAP batches consist of two media sources per batch: idx=[0,1]
        inputs:
            K       - number of nearest neighbors to return [5]
            crop_pos - crop media at 0+crop_pos, END-crop_pos seconds [300]
            end_pos  - position (in seconds) of last query to process [3600]
        """
        self.K = K if K is not None else self.K # NN to keep
        end_qpos = end_pos * self.FR # End Qpos in frames
        crop_spos = int( crop_pos * self.FR )
        self.nnMerged = []
        nnMerged = self.nnMerged
        for k, nnResult in enumerate(self.nnRaw):
            if len(nnResult):
                idx0 = nnResult['idx']==0
                if any(idx0):
                    s0_mx=max(nnResult['spos'][where(idx0)])
                    d0 = nnResult[idx0]
                    d0 = d0[where((d0['spos']>crop_spos) & (d0['spos']<s0_mx-crop_spos))]
                idx1 = nnResult['idx']==1
                if any(idx1):
                    s1_mx=max(nnResult['spos'][where(idx1)])
                    d1 = nnResult[idx1]
                    d1 = d1[where((d1['spos']>crop_spos) & (d1['spos']<s1_mx-crop_spos))]
                if len(d0) and len(d1):
                    bar = rfn.stack_arrays([d0,d1])
                elif len(d0):
                    bar = d0
                elif len(d1):
                    bar = d1
                else:
                    continue
                bar['dist'][where(isnan(bar['dist']))]=2
                bar['prob'][where(isnan(bar['prob']))]=0
                bar['amp'][where(isnan(bar['amp']))]=0
                bar['idx'] = bar['idx'] + 2*k
                nnMerged.append(bar)
        nnMerged = rfn.stack_arrays(nnMerged)
        nnMerged = nnMerged[where(nnMerged['dist']!=nan)]
        nnMerged = np.sort(nnMerged,kind='mergesort',order=['qpos','dist'])
        nnMerged = [nnMerged[where(nnMerged['qpos']==k)][:K].data for k in arange(0,nnMerged['qpos'].max()+1,self.shingle_hop)]
        nnMerged = rfn.stack_arrays(nnMerged)
        nnMerged = nnMerged[where(nnMerged['qpos']<=end_qpos)]
        self.nnMerged = nnMerged
        savetxt(out_fname,nnMerged,fmt=self.fields_str)

    def reduce_nearest_neighbor_variations(self, K=5, out_fname='movievariations.imatsh.txt', json_dir='actiondata', **kwargs):
        """
        REDUCE nearest neighbor results from batch files in per-variation subsets
        Assumes that MAP batches consist of two media sources per batch: idx=[0,1]
        inputs:
            K          - K nearest neighbors to return [5]
            json_dir   - film metadata store ['actiondata']
            **kwargs:
              crop_pos - time locators in seconds [300]
        """        
        self.K = K if K is not None else self.K # NN to keep
        end_qpos = int( 3600 * self.FR )
        newmovs, Ao, directors, years = pickle.load(open('oms_db.pickle'))
        metadata = [json.load(open(json_dir+os.sep+f+'.json')) for f in newmovs]
        F = acts.FilmDB()
        AoA = F.as_structured_array(Ao) # Action OneMillionSeconds set as Array
        years = AoA['year']
        yidx = np.argsort(years) # ordered by years
        yAoA = AoA.take(yidx)
        nfilms_per_director = np.array([sum(AoA['director']==d) for d in directors])
        director_idx_by_nfilms = np.argsort(nfilms_per_director)[::-1][:10] # Take top 10 directors
        directors = directors[director_idx_by_nfilms]
        director_idx_by_nfilms_and_years = [yidx[where(yAoA['director']==d)] for d in directors]        
        self.nnMerged = []
        self.var_count = 0
        self._reduce_variations_from_idx_l([range(len(newmovs))], metadata, 0, **kwargs)
        self._reduce_variations_from_idx_l(director_idx_by_nfilms_and_years, metadata, 1, **kwargs)
        self._reduce_variations_from_idx_l(yidx.reshape(-1,6), metadata, len(directors) + 1, **kwargs)
        nnMerged = self.nnMerged
        nnMerged = rfn.stack_arrays(nnMerged)
        nnMerged = nnMerged[where(nnMerged['dist']!=nan)]
        nnMerged = np.sort(nnMerged,kind='mergesort',order=['qpos','dist'])
        nnMerged = [nnMerged[where(nnMerged['qpos']==k)][:K].data for k in arange(0,nnMerged['qpos'].max()+1,self.shingle_hop)]
        nnMerged = rfn.stack_arrays(nnMerged)
        nnMerged = nnMerged[where(nnMerged['qpos']<=end_qpos)]
        self.nnMerged = nnMerged
        savetxt(out_fname, nnMerged, fmt=self.fields_str)

    def _reduce_variations_from_idx_l(self, film_idx_list, metadata, var_offset=0, crop_pos=300):
        crop_spos = int( crop_pos * self.FR )
        nnMerged = self.nnMerged
        num_variations = len(film_idx_list)
        variation_limits = self.get_variation_limits()[var_offset:var_offset+num_variations]
        for di, var_lim in enumerate(variation_limits):
            start_qpos, end_qpos = var_lim[0]/int(self.FH), var_lim[1]/int(self.FH)
            print self.var_count, start_qpos, end_qpos
            self.var_count += 1
            films = film_idx_list[di]
            for film in films: # only one film per pass
                nnResult = self.nnRaw[film/2]
                nnResult = nnResult[where(nnResult['idx']==film%2)]
                nnResult = nnResult[where((nnResult['qpos']>=start_qpos) & (nnResult['qpos']<end_qpos))]
                if len(nnResult):
                    spos_mx = int( metadata[film]['length'] * self.FR )
                    results = nnResult[where((nnResult['spos']>=crop_spos) & (nnResult['spos']<spos_mx-crop_spos))]
                    results['dist'][where(isnan(results['dist']))]=2
                    results['prob'][where(isnan(results['prob']))]=0
                    results['amp'][where(isnan(results['amp']))]=0
                    results['idx'] = film
                    nnMerged.append(results)

    def get_variation_limits(self):
        try:
            f = open('variation_limits.pickle')
            variation_limits = pickle.load(f)
        except IOError:
            locs,sums = normalize_target_audio(proc_audio=False)
            locs = np.array(locs)
            variation_limits = np.c_[locs[:-1],locs[1:]]
            with open('variation_limits.pickle','w') as f:
                pickle.dump(variation_limits,f)
        return variation_limits

# Helper functions
def dB(x):
    return 20*np.log10(x)

def normalize_target_audio(input_file='moviehires_endpos_beta02.imatsh.wav', 
                           sources_expr='/home/mkc/Music/GoldbergVariations/*48_1.wav', write_me=False, amp_factor=0.5, proc_audio=True):
    """
    Per-variation normalization of concatenated imatsh file using individual sources as locators
    Assumes that the input_file and the source_dir have the same sample rate
    inputs:
        input_file  - the file to be processed (locally normalized)
        sources_expr- regular expression for input files
        write_me    - write output files when true [False]
        amp_factor  - amplitude change factor (proportion of full scale normalization) [0.5]
        proc_audio  - whether to process target audio using source audio info [1]
    outputs:
        sample_locators - sample locators for each variation
        audio_summaries - min, max, rms values for each variation        
    output files:
        output_file = {input_file_stem}+'norm.'+{input_ext}
    """
    # Compute min, max, rms per source file
    flist = glob.glob(sources_expr)
    flist.sort()
    sample_locators = [0]
    audio_summaries = []
    ext_pos = input_file.rindex('.')
    outfile_stem, ext = input_file[:ext_pos], input_file[ext_pos+1:]
    for i,f in enumerate(flist):
        x,sr,fmt = skaud.wavread(f)
        print f, sr, fmt
        if(len(x.shape)>1):
            x = x[:,0] # Take left-channel only
        sample_locators.extend([len(x)])
        audio_summaries.append([max(abs(x)), np.sqrt(np.mean(x**2))])
        if proc_audio:
            y,sr_y,fmt_y = skaud.wavread(input_file, first=np.cumsum(sample_locators)[-2], last=sample_locators[-1])
            if sr != sr_y:
                raise ValueError("input and source sample rates don't match: %d,%d"%(sr,sr_y))
            audio_summaries.append([max(abs(y[:,0])), np.sqrt(np.mean(y[:,0]**2))])
            max_val = audio_summaries[-1][0]
            rms_val = audio_summaries[-1][1]
            norm_cf = amp_factor / max_val + (1 - amp_factor)
            outfile = outfile_stem+'_%02d.%s'%(i+1,ext)
            max_amp_val = norm_cf * max_val
            rms_amp_val = norm_cf * rms_val
            print '%s: nrm=%05.2fdB, peak=%05.2fdB, *peak=%05.2fdB, rms=%05.2fdB, *rms=%05.2fdB'%(
                outfile, dB(norm_cf), dB(max_val), dB(max_amp_val), dB(rms_val), dB(rms_amp_val))
            if(write_me):
                skaud.wavwrite(norm_cf*y, outfile, sr, fmt)
    return np.cumsum(sample_locators), np.array(audio_summaries)

