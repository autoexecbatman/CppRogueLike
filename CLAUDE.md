# C++ RogueLike - Development Guidelines

## Project Context
C++ roguelike game using ncurses for terminal-based gameplay.

## Architecture Principles
Follow principles from E:\architecture_docs\v10.9.4\essential:
- Small atomic steps with build verification
- Anti-brute force: Design prevention over detection
- Single source of truth via GameContext
- No marketing language or promotional tone
- Professional factual communication

## Architecture Status

### GameContext Dependency Injection
**Status: COMPLETE**

The GameContext refactoring has been completed:
- GameContext.h defines system dependency container
- Game::get_context() populates context pointers
- System methods accept GameContext& parameter
- Systems use ctx-> instead of global game object
- No global extern Game game declarations in codebase

Remaining "game." references are legitimate:
- main_C++RogueLike.cpp: Entry point where Game object is instantiated
- String literals: "saving the game", "game.sav"
- Comments: Historical references

## Development Rules
1. Small atomic changes with verification after each step
2. Commit after each successful unit of work
3. No override patterns: Design prevention at source
4. **NEVER add Co-Authored-By lines to commits**
5. Pass GameContext& to all system methods
6. Use ctx-> to access game systems

## Code Standards
- Const correctness enforcement
- Type safety via GameContext
- No decorative characters in code
- ASCII only for performance
- Proper const qualification
- Single responsibility principle
- Clear architectural boundaries

## Reference
Architecture: E:\architecture_docs\v10.9.4\essential
Pattern: Dependency injection via GameContext
