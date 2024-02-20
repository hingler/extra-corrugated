import os

try:
  env = Import('env')
except:
  env = Environment()
  env.Append(CXXFLAGS=["-std=c++17"])

  env.Tool('compilation_db')
  cdb = env.CompilationDatabase()
  Default(cdb)




env.Append(CPPPATH=[[env.Dir(d) for d in [
  "include",
  "lib/glm"
]]])

test_dir = "test/"
tests = [
  "TestProg"
]

test_sources = [test_dir + test + ".cpp" for test in tests]
for test in test_sources:
  Default(env.Program("g", test))
