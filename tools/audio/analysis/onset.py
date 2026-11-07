import numpy as np, features as F
def spec(x):
    N=F.NFFT; H=F.HOP
    fr=1+(len(x)-N)//H
    idx=np.arange(N)[None,:]+H*np.arange(fr)[:,None]
    return np.abs(np.fft.rfft(x[idx]*F._window,axis=1))
def onset(x):
    s=np.log1p(spec(x)*10)
    f=np.maximum(0,np.diff(s,axis=0)).sum(axis=1)
    f=f-np.convolve(f,np.ones(25)/25,'same')
    return (f-f.mean())/(f.std()+1e-9)
def local_lags(r, m, win=200, step=100, lo=-150, hi=1500):
    out=[]
    for s0 in range(0, len(r)-win, step):
        a=r[s0:s0+win]
        best=(-9,0)
        for lag in range(lo,hi):
            b0=s0+lag
            if b0<0 or b0+win>len(m): continue
            v=float(np.dot(a,m[b0:b0+win])/win)
            if v>best[0]: best=(v,lag)
        out.append((s0/50, round(best[0],2), best[1]/50))
    return out
