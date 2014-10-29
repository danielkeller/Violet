#! /usr/bin/python

import os
import subprocess
import cPickle as pickle
import hashlib

cflags = ['-g', '-Wall', '-Wno-missing-braces', '-Werror', '-pedantic', '-I.', '-IViolet', '-ferror-limit=3', '-O2']
cppflags = ['-std=c++11']
sourcedirs = ['Violet', 'GL', 'Lodepng']
libs = ['-lGL', '-lGLU', 'GLFW/libglfw3.a', '-lX11', '-lXxf86vm', '-lpthread', '-lXrandr', '-lXi', '-lXinerama', '-lXcursor']
executable = 'violet'

def makedir(dir):
    return [os.path.join(path,fname) for path,_,fnames in os.walk(dir) for fname in fnames
        if fname.endswith('.cpp') or fname.endswith('.c')]

def flags(fname):
    if fname.endswith('.cpp'):
        return cflags + cppflags
    else:
        return cflags

def cxx(fname):
    if fname.endswith('.c'):
        return 'clang'
    else:
        return 'clang++'

def command(fname):
    return [cxx(fname), fname] + flags(fname)

def obj(fname):
    return os.path.join('obj', fname.replace('.cpp', '.o').replace('.c', '.o'))

if not os.path.isfile('sums'):
    with open('sums','wb') as hdl:
        pickle.dump({}, hdl, -1)

with open('sums','rb') as hdl:
    oldsums = pickle.load(hdl)

sums = {}
procs = {}
sources = [src for dirname in sourcedirs for src in makedir(dirname)]
for fname in sources:
    sums[fname] = hashlib.md5(subprocess.check_output(command(fname) + ['-E'])).hexdigest()
    if not fname in oldsums or not oldsums[fname] == sums[fname]:
        if not os.path.isdir(os.path.dirname(obj(fname))):
            os.makedirs(os.path.dirname(obj(fname)))
        cmd = command(fname) + ['-c', '-o', obj(fname)]
        print ' '.join(cmd)
        procs[fname] = subprocess.Popen(cmd)

sums = {fname:cksum for (fname,cksum) in sums.iteritems() if not fname in procs or procs[fname].wait() == 0}
#print sums

with open('sums','wb') as hdl:
    pickle.dump(sums, hdl, -1)

cmd = [cxx(''), '-o', executable] + [obj(src) for src in sources] + cflags + libs
print ' '.join(cmd)
subprocess.Popen(cmd).wait()
