import os
import lit

import lit.llvm

config.name = 'LLVM FP Tools'
config.suffixes = ['.ll']
config.test_format = lit.formats.ShTest()

config.test_source_root = os.path.dirname(__file__)
