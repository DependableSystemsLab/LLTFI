$LLFI_BUILD_ROOT/tools/tracediffML llfi/baseline/llfi.stat.trace.prof.txt $1 > diffReport
$LLFI_BUILD_ROOT/tools/traceontograph diffReport llfi.stat.graph.dot > diffgraph.dot

