"""build_ispa.py orig|core: the ispa grammar parser (parser/ispa), generated with the C++ ispastdlib or `--stdlib core`."""
import sys, os; sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import suite
R=os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
os.environ['SUITE_GEN']='--dep python '+R+'/parser/python/bootstrapped/PyRawParser'
os.environ['SUITE_SRC']=R+'/parser/python/bootstrapped/PyRawParser.cpp '+R+'/parser/python/extern.cpp'
os.environ['SUITE_INC']=R+'/parser/python/bootstrapped '+R+'/parser/python '+R+'/parser/ispa '+R+'/parser'
os.environ['REBUILD']='1'
which=sys.argv[1]
b=suite.build_cpp('Parser', R+'/parser/ispa/entry.isc', R+'/parser/ispa', stdlib=(which=='core'))
print(which, b)
