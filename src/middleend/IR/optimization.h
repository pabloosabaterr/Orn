/**
 * @file optimization.h
 * @brief IR optimization pass declarations.
 *
 * Exposes constantFolding(), copyProp(), deadCodeElimination(),
 * and the top-level optimizeIR() entry point. Include from modules
 * that need to run optimizations on the IR before code generation.
 */

void constantFolding(IrContext *ctx);
void copyProp(IrContext *ctx);
void deadCodeElimination(IrContext *ctx);
void optimizeIR(IrContext *ctx, int optLvl);