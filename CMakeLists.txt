#set(LLVM_LINK_COMPONENTS
#        Support
#        )
#
#add_clang_library(XPSAnalysis
#        PointerAnalysis/HelloPointer.cpp
#        LoopAnalysis/HelloLoop.cpp
#        LINK_COMPONENTS
#        ${LLVM_TARGETS_TO_BUILD}
#        Core
#        Support
#        TransformUtils
#  )



add_llvm_loadable_module(XPSAnalysis
        PointerAnalysis/HelloPointer.cpp
        LoopAnalysis/HelloLoop.cpp
        LoopInfo/DomSetPass.h
        LoopInfo/LoopInfo.cpp
        PointerAnalysis/InstSetName.h

        PLUGIN_TOOL
        opt
        )