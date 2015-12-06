# ratchet

Ratchet is a dynamically typed language aiming for:

  * Clean syntax
  * Efficiency
  * Concurrency

There's currently not much to see beyond the messy beginnings of a compiler/VM.

Influences include:

  * Go: goroutines, channels
  * Lua: register-based VM, closure implementation based on upvals
  * Smalltalk: message passing, extensible classes
  * Clojure: immutable collections
  * JSX: embedded HTML/XML
  * Potion: data language
  * Ruby: symbols
  * Ian Piumarta's research on Open, Extensible Object Models

Other planned features:

  * hot module reloading
  * support multiple OS threads without GIL
  * embedded grammars for DSLs
  * concise syntax for arrays, maps, sets, vectors/matrices, tuples and colors
