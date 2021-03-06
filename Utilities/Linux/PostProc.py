# -*- coding: utf-8 -*-
import glob , os , shutil,string
from math import cos,pi,exp
from numpy import *
from scitools.easyviz import *
import scitools.filetable as ft
import sys
import matplotlib
#matplotlib.use('GTK')
def gaussNodes(m,tol=10e-9):
    if (m==1):
        A = zeros(m)   
        x = zeros(m)   
        x[0] = 0
        A[0] = 1
    else:    
        def legendre(t,m):
           
            p0 = 1.0; p1 = t
            for k in range(1,m):
                p = ((2.0*k + 1.0)*t*p1 - k*p0)/(1.0 + k )
                p0 = p1; p1 = p
            dp = m*(p0 - t*p1)/(1.0 - t**2)
            return p,dp
        
        A = zeros(m)   
        x = zeros(m)   
        nRoots = (m + 1)/2          # Number of non-neg. roots
        for i in range(nRoots):
            t = cos(pi*(i + 0.75)/(m + 0.5))  # Approx. root
            for j in range(30): 
                p,dp = legendre(t,m)          # Newton-Raphson
                dt = -p/dp; t = t + dt        # method         
                if abs(dt) < tol:
                    x[i] = t; x[m-i-1] = -t
                    A[i] = 2.0/(1.0 - t**2)/(dp**2) # Eq.(6.25)
                    A[m-i-1] = A[i]
                    break
        #x = 1.0
    return x,A
def f(x):
    return exp(x)

NElem = 3;
NGauss = 6;
NZ = NElem*NGauss;
NXSections = 11;
X = linspace(1,10,NXSections);
Width = 3.0;
dz = Width/NElem;
z = zeros((NElem,NGauss));
wgt = zeros((NElem,NGauss));
Z = zeros(NZ);
xi,w = gaussNodes(NGauss,tol=1e-14)
sum = 0;
counter = 0;
for n in range(0,NElem,1):
    for k in range(0,NGauss,1):
       
        z0 = -1.5 + (n)*dz;
        #print z0;
        z[n,k] = z0 +  (-xi[k]+1)*dz/2;
        wgt[n,k] = w[k]*dz/2;
        print z[n,k],wgt[n,k];
        sum = sum + wgt[n,k]*f(z[n,k]);
        Z[counter] = z[n,k];
        counter = counter+1;
        

file = open('points.mesh' , 'w')
file.write('%12g \n'  % (NXSections*NZ))
for i in range(0,NXSections,1):
    for k in range(0,NZ,1):
        file.write('%12.8e %12.8e %12.8e \n'  % (X[i],0.0,Z[k]))

file.close()    
os.system( "./extract -n 5  -m points.mesh -r jet-laminar.rea jet.fld > result.out")


file = open('result.out' , 'r')
file.readline()
X,Y,Z,U,V,W,P,T =ft.read_columns(file); 
#for i in xrange(X.shape[0]):
    #print X[i],T[i]

Tavg = zeros(NXSections);
counter = 0;
for i in range(0,NXSections,1):
    print '-------------------------------'
    for n in range(0,NElem,1):
	for k in range(0,NGauss,1):
	  
	    
	    Tavg[i] = Tavg[i] +  T[counter]*wgt[n,k];
	    print '%8.5f  %8.5f ' % (T[counter],Tavg[i]/3)
	    counter = counter + 1

Tavg = Tavg/Width;
	    
#for i in range(0,NXSections,1):
    #print Tavg[i]

#t = linspace(0,3,51);
#y = t**2*exp(-t**2);
#plot(t,y)

#xi,w = gaussNodes(5,tol=1e-14)
#for k in range(0,5,1):
#    print k,w[k],xi[k],f(xi[k])

#Makedir('MEgPC')
#BR_s = 0.5
#BR_e = 1.5
#L = BR_e - BR_s
#NElem = 3
#P = 4
#for i in range(1,NElem+1,1):
#    newdir = os.path.join('MEgPC','NElem='+str(i))    
#    Makedir(newdir)
#    print '####################################'
#    print i
#    for j in range(1,i+1,1):
#        print '---------------------------'    
#        a = BR_s + (j-1)*1.0/i*L
#        b = BR_s + j*1.0/i*L
#        xi,w = gaussNodes(P+1,tol=1e-14)
#        for k in range(0,P+1,1):
#
#            BR = ((1-xi[k])*a + (1+xi[k])*b)/2.0
#            print k,xi[k] , BR
#            newdir = os.path.join('MEgPC','NElem='+str(i),'Elem='+str(j)+'-Q='+str(xi[k]) )
#            Makedir(newdir)
#            m = 0
#            ifile = open(infilename , 'r')
#            ofile = open('jet-laminar.rea' , 'w')
#            for line in ifile:
#                m = m+1
#                #print m,line
#                if m==10:
#                    line = '%14.8e       BR  \n' % BR
#                ofile.write(line)
#            ofile.close()
#            ifile.close()
#            file_tmp =  os.path.join(newdir,'jet-laminar.rea')
#            #file_tmp = open(file_tmp , 'w')
#            print newdir
#            shutil.copy(os.path.join(os.curdir,'jet-laminar.rea'),file_tmp)
#            file_tmp =  os.path.join(newdir,'nektar')
#            shutil.copy(os.path.join(os.curdir,'nektar'),file_tmp)
#            file_tmp =  os.path.join(newdir,'sample.pbs')
#            shutil.copy(os.path.join(os.curdir,'sample.pbs'),file_tmp)
         #   line =  'cd '+'/work/hbabaee/Case/BackStep-UQ/' + newdir+ '\n' 
          #  script.write(line)
           # line =  'qsub sample.pbs'+ '\n'
           # script.write(line)
#ifile.close() ; ofile.close()                       
