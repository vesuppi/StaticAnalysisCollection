set(LLVM_LINK_COMPONENTS
        Support
        )

add_clang_library(XPSAnalysis
        PointerAnalysis/HelloPointer.cpp

        LINK_COMPONENTS
        ${LLVM_TARGETS_TO_BUILD}
        Core
        Support
        TransformUtils
  )

