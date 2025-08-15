# 🃏 Pipes & Decks — A Multi‑Process Card Game in C

> **Deal. Fork. Play.**  
> A concurrent card‑dealing engine built in pure C, using UNIX processes, pipes, and a dash of competitive card‑table spirit.

---

## 📖 Overview

**Pipes & Decks** is a concurrent card distribution and gameplay simulator written in **C**.  
It demonstrates:

- **UNIX Process Control** → `fork()` to spawn player processes  
- **Inter‑Process Communication (IPC)** → `pipe()` to pass game state between parent and children  
- **Custom Sorting Logic** → suit‑then‑rank ordering  
- **Duplicate Detection** → ensuring clean decks before play

While designed as a fun card game, this is really a **hands‑on lab** for low‑level systems programming concepts.

---

## 🛠 Features

- **Parent–Child Architecture**
  - Parent acts as dealer and game coordinator
  - Each child process is an independent *player*

- **Suit & Rank Mapping**
  - Suits mapped to integers: ♦=0, ♣=1, ♥=2, ♠=3  
  - Ranks mapped to values: `2` high down to `3` low

- **Gameplay Flow**
  - Game starts with ♦3  
  - Player must beat the last played card or pass  
  - Child announces completion when out of cards

- **Robust Input Handling**
  - Reads from `stdin`  
  - Rejects invalid suits/ranks  
  - Discards duplicates with a warning

---

## 📂 Suggested Code Structure

> *Currently implemented in a single file; can be modularized for clarity.*

```text
├── card.h / card.c        # Card struct, mapping, and comparison logic
├── player.c               # Child process gameplay loop
├── dealer.c               # Parent process orchestration
└── README.md
