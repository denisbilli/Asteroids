# Asteroids (C++)

Welcome to the repository of the C++ version of the **Asteroids** game! This project represents my implementation of the classic arcade video game, developed for Windows systems using a custom 2D graphics engine.

## Key Features

- **Custom 2D graphics engine**: Developed from scratch to handle geometric shapes, collisions, and smooth movement.
- **Gameplay inspired by the original**: Control a spaceship to destroy asteroids and avoid collisions.
- **Extensibility**: Designed to be a testbed for further features and improvements.
- **Runtime graphics**: All visuals are rendered using geometric shapes without external assets or sprites.
- **Open Source**: Developed with an open mindset, inviting contributions and feedback.

## Project Goals

This project was born as a personal exercise to improve my skills in:

- **Advanced C++**: Memory management, polymorphism, and design patterns.
- **Game Development**: Implementation of game logic, physics, and visual interactions.
- **Windows-specific development**: Leveraging WinAPI and GDI+ for graphical rendering.

## Prerequisites

### Required Software

- **Windows OS**: The game is designed for Windows systems.
- **C++ Compiler**: Support for C++17 or later.
- **Graphics Libraries**:
  - GDI+ (included in Windows SDK).
  - [cURL](https://curl.se/) (for online leaderboard functionality).
  - [nlohmann/json](https://github.com/nlohmann/json) (for JSON handling).

### Dependencies

Ensure the following libraries are available:
- GDI+ (part of Windows SDK).
- cURL.
- nlohmann/json.

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/denisbilli/Asteroids.git
   cd Asteroids
   ```

2. Open the project in Visual Studio or your preferred Windows C++ IDE.

3. Build the solution/project using the provided `.sln` or configure manually.

4. Run the game by launching the compiled executable.

## Project Structure

- `src/`: Contains the game source code.
- `build/`: Directory for compiled files (not included in the repo).
- `README.md`: This file.

## Game Controls

- **Up arrow**: Accelerate
- **Left/right arrows**: Rotate the spaceship
- **Spacebar**: Shoot
- **P**: Pause/unpause the game
- **F11**: Toggle debug information

## Contributions

Contributions, feedback, and bug reports are welcome! Please follow these steps:

1. Fork the repository.
2. Create a branch for your change:
   ```bash
   git checkout -b feature-name-of-feature
   ```
3. Submit a pull request.

## Roadmap

- [ ] Enhance collision detection and handling.
- [ ] Introduce a more dynamic difficulty scaling system.
- [ ] Add online leaderboard functionality.
- [ ] Optimize rendering performance for high-resolution displays.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.

## Contacts

For more information or to discuss the project, you can contact me through my [GitHub profile](https://github.com/denisbilli) or via LinkedIn.
