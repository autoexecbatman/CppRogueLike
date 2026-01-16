# C++ RogueLike - Agent Execution Guidelines

## Project Context
C++ roguelike game using ncurses. Currently in Phase C: Global game object elimination via dependency injection pattern.

## Architecture Compliance
Follow principles from E:\architecture_docs\v10.9.4\essential:
- Small atomic steps with build verification
- Anti-brute force: Design prevention over detection
- Single source of truth via GameContext
- No marketing language or promotional tone
- Professional factual communication

## Current Phase: GameContext Dependency Injection

### Objective
Eliminate global extern Game game object by refactoring to dependency injection pattern.

### Strategy
1. GameContext struct contains pointers to all Game systems
2. Game::get_context() populates context
3. Systems receive GameContext& parameter
4. Replace game.X with ctx->X incrementally
5. Build verification after each file
6. Backward-compatible wrappers during migration

### Progress
- Phase 1: GameContext.h created (57 lines, 20 forward declarations)
- Phase 2: Game::get_context() implemented
- Phase 3: MenuManager refactored (1 game. ref eliminated)
- Phase 4: HungerSystem refactored (14 game. refs eliminated)

### Current Status
- Build: PASSING
- Systems refactored: 2 of 48 files
- References remaining: ~1,183 game. usages across 46 files

### Next Targets
System managers with fewest dependencies:
- TargetingSystem (18 refs)
- GameLoopCoordinator
- CreatureManager

### Execution Rules
1. Small steps: One system at a time
2. Build verification: After each refactoring
3. Commit frequency: Every successful refactoring
4. No override patterns: Design prevention at source
5. Backward compatibility: Temporary wrappers until migration complete
6. **NEVER add Co-Authored-By lines to commits**

### Quality Standards
ISO quality compliance:
- Const correctness enforcement
- Type safety via GameContext
- Compilation error prevention
- Zero technical debt accumulation
- Professional code standards

## Code Review Guidelines
- No decorative characters in code
- ASCII only for performance
- Proper const qualification
- Single responsibility principle
- Clear architectural boundaries

## Reference
Architecture: E:\architecture_docs\v10.9.4\essential
Pattern: Dependency injection via GameContext
Goal: Eliminate global state, enable testability
