void constantFolding(IrContext *ctx);
void copyProp(IrContext *ctx);
void deadCodeElimination(IrContext *ctx);
void optimizeIR(IrContext *ctx, int optLvl);