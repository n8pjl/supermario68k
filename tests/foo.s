	.file	"foo.c"
	.functype	render (i32, i32) -> ()
	.section	.text.render,"",@
	.hidden	render                          # -- Begin function render
	.globl	render
	.type	render,@function
render:                                 # @render
	.functype	render (i32, i32) -> ()
	.local  	i32, i32, i32, i32, i32, i32, i32
# %bb.0:
	i32.const	0
	local.set	2
.LBB0_1:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB0_2 Depth 2
	loop    	                                # label0:
	local.get	2
	i32.const	5
	i32.shl 
	i32.const	renderbuf
	i32.add 
	local.set	3
	local.get	1
	local.get	2
	i32.add 
	i32.load8_u	0
	i32.const	255
	i32.xor 
	local.set	4
	local.get	0
	local.get	2
	i32.add 
	i32.load8_u	0
	i32.const	255
	i32.xor 
	local.set	5
	i32.const	0
	local.set	6
.LBB0_2:                                #   Parent Loop BB0_1 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	loop    	                                # label1:
	local.get	3
	local.get	6
	i32.const	2
	i32.shl 
	i32.add 
	local.tee	7
	i32.const	255
	i32.store8	3
	local.get	7
	i32.const	85
	i32.const	0
	local.get	5
	local.get	6
	i32.const	7
	i32.xor 
	local.tee	8
	i32.shr_u
	i32.const	1
	i32.and 
	i32.select
	i32.const	-86
	i32.const	0
	local.get	4
	local.get	8
	i32.shr_u
	i32.const	1
	i32.and 
	i32.select
	i32.or  
	local.tee	8
	i32.store8	2
	local.get	7
	local.get	8
	i32.store8	1
	local.get	7
	local.get	8
	i32.store8	0
	local.get	6
	i32.const	1
	i32.add 
	local.tee	6
	i32.const	8
	i32.ne  
	br_if   	0                               # 0: up to label1
# %bb.3:                                #   in Loop: Header=BB0_1 Depth=1
	end_loop
	local.get	2
	i32.const	1
	i32.add 
	local.tee	2
	i32.const	3840
	i32.ne  
	br_if   	0                               # 0: up to label0
# %bb.4:
	end_loop
                                        # fallthrough-return
	end_function
                                        # -- End function
	.hidden	renderbuf                       # @renderbuf
	.type	renderbuf,@object
	.section	.bss.renderbuf,"",@
	.globl	renderbuf
	.p2align	4, 0x0
renderbuf:
	.skip	122880
	.size	renderbuf, 122880

	.ident	"clang version 24.0.0git (/startdir/llvm-project c0125a7bf833b6cf0d5b4a085b63094e0893c85a)"
	.section	.custom_section.producers,"",@
	.int8	1
	.int8	12
	.ascii	"processed-by"
	.int8	1
	.int8	5
	.ascii	"clang"
	.int8	75
	.ascii	"24.0.0git (/startdir/llvm-project c0125a7bf833b6cf0d5b4a085b63094e0893c85a)"
	.section	.bss.renderbuf,"",@
	.section	.custom_section.target_features,"",@
	.int8	8
	.int8	43
	.int8	11
	.ascii	"bulk-memory"
	.int8	43
	.int8	15
	.ascii	"bulk-memory-opt"
	.int8	43
	.int8	22
	.ascii	"call-indirect-overlong"
	.int8	43
	.int8	10
	.ascii	"multivalue"
	.int8	43
	.int8	15
	.ascii	"mutable-globals"
	.int8	43
	.int8	19
	.ascii	"nontrapping-fptoint"
	.int8	43
	.int8	15
	.ascii	"reference-types"
	.int8	43
	.int8	8
	.ascii	"sign-ext"
	.section	.bss.renderbuf,"",@
