# qt-hsharp

Bindingi **Qt6 Widgets** dla języka **[H#](https://github.com/HackerOS-Linux-System/H-Sharp)**,
publikowane jako pakiet **[bytes](https://github.com/HackerOS-Linux-System/bytes)**.

Biblioteka wystawia idiomatyczne, wysokopoziomowe API H# (`qt::init()`,
`qt::new_window()`, `qt::new_button()`, `qt::poll_event()`, ...) osadzone
na niewielkim, statycznie linkowanym shimie C++ (`extern static [c, "qthsharp"]`),
który sam w sobie używa prawdziwego Qt6.

---

## Dlaczego shim, a nie bezpośrednie `extern [c++]` na klasach Qt?

H# potrafi wołać przez `extern` wyłącznie zwykłe, niemanglowane symbole C
(`extern "C"`) — nie rozumie layoutu klas C++, vtabli, przeciążeń ani
manglingu nazw Qt. Dlatego:

1. `native/src/qthsharp.cpp` implementuje mały, płaski **C ABI**
   (`native/include/qthsharp.h`) — same funkcje `extern "C"`, żadnych
   klas C++ na granicy.
2. `src/qt.h#` deklaruje ten sam ABI przez
   `extern static [c, "qthsharp"]` — zgodnie z konwencją, jaką H#
   już stosuje dla `malloc`/`free` w swoim README (uchwyt = wskaźnik
   zrzutowany na `int`).
3. Wokół surowego `extern` `src/qt.h#` buduje wygodne, bezpieczne API
   H# (struktury `Widget`/`Layout`, enum `EventKind`, funkcje `pub fn`).

### Sygnały Qt a brak wskaźników funkcyjnych w H#

H# nie ma typu "wskaźnik do funkcji" / domknięcia, który dałoby się
przekazać przez `extern`. Zamiast tego `qthsharp.cpp` **kolejkuje**
zdarzenia (kliknięcia, zmiany tekstu, zamknięcie okna, ...) wewnętrznie
i H# odbiera je zwykłym pollingiem:

```hsharp
let ev = qt::poll_event()   ;; Event? — nil, jeśli kolejka pusta
if ev != nil is
    match ev.kind is
        qt::EventKind::Clicked => is write("kliknięto!") end
        _ => is end
    end
end
```

To zwykły, w pełni H#-owy `match` na enumie — żadnego mostkowania
domknięć przez granicę FFI.

---

## Struktura repozytorium

```
qt-hsharp/
├── Bytes.hk                 ← manifest pakietu bytes (biblioteka)
├── src/
│   └── qt.h#                ← extern static [c, "qthsharp"] + API wysokiego poziomu
├── native/
│   ├── include/qthsharp.h   ← nagłówek C ABI (źródło prawdy dla sygnatur)
│   ├── src/qthsharp.cpp     ← implementacja C++ nad Qt6 Widgets
│   ├── CMakeLists.txt       ← buduje statyczne native/libqthsharp.a
│   └── build.sh             ← ./build.sh — jedna komenda, jeden wynik
├── examples/
│   ├── Bytes.hk             ← manifest projektu-przykładu (zależy od qt-hsharp)
│   ├── hello_window.h#      ← najprostsze okno z etykietą
│   └── counter_app.h#       ← przycisk + pole tekstowe + pętla poll_event
└── tests/
    └── smoke_test.h#        ← test bez GUI: sam FFI (init/quit), pod `bytes test`
```

---

## Wymagania

- **H#** ≥ 0.9 (kompilator `h#` + `bytes`) — LLVM 21 backend
- **Qt6** (moduł Widgets) + nagłówki deweloperskie
- **CMake** ≥ 3.16, kompilator C++17, **pkg-config**

Instalacja zależności systemowych:

```bash
# Debian / Ubuntu
sudo apt install qt6-base-dev cmake build-essential pkg-config

# Fedora
sudo dnf install qt6-qtbase-devel cmake gcc-c++ pkgconf-pkg-config

# Arch Linux
sudo pacman -S qt6-base cmake pkgconf base-devel
```

---

## Budowanie

### 1. Zbuduj natywny shim (`libqthsharp.a`)

```bash
cd native
./build.sh          # Release (domyślnie); ./build.sh Debug dla wersji debug
```

Skrypt uruchamia CMake + Qt6 i zostawia gotowe `native/libqthsharp.a`
(oraz nagłówek `native/include/qthsharp.h`) dokładnie tam, gdzie
oczekuje go blok `extern static [c, "qthsharp"]` w `src/qt.h#`.

### 2. Zbuduj samą bibliotekę H# (opcjonalnie, do sprawdzenia typów)

```bash
bytes build          # z katalogu głównego repo — czyta Bytes.hk
# albo bezpośrednio:
h# check src/qt.h#
```

### 3. Uruchom przykład

```bash
cd examples
bytes run                       # domyślnie hello_window.h# (patrz Bytes.hk -> entry)
```

lub bez `bytes`, bezpośrednio kompilatorem H#:

```bash
h# compile examples/hello_window.h# \
    -I . \
    --flags "-Lnative -lqthsharp" \
    -o hello_window
./hello_window
```

> **Uwaga:** `native/libqthsharp.a` to archiwum statyczne — nie niesie
> ze sobą własnych zależności Qt (`.so`) tak, jak robi to biblioteka
> dynamiczna. Dlatego `src/qt.h#` dokłada pusty blok
> `extern dynamic [c++, "Qt6Widgets"]`, którego jedynym zadaniem jest
> wstrzyknięcie flag linkera przez `pkg-config --libs Qt6Widgets`
> (co samo w sobie ciągnie `Qt6Core`/`Qt6Gui` przez zależności w
> plikach `.pc`) oraz automatyczne `-lstdc++` dla bloków `[c++]`. Dzięki
> temu wystarczy jedno `mod qt` w kodzie H# — nie trzeba ręcznie
> doklejać flag Qt przy każdej kompilacji.

---

## Szybki start

```hsharp
mod qt

fn main() is
    qt::init()

    let window:    qt::Widget = qt::new_window("Moja aplikacja", 400, 300)
    let container: qt::Widget = qt::new_container()
    let label:     qt::Widget = qt::new_label("Cześć, Qt6!")

    let layout: qt::Layout = qt::new_vbox()
    qt::layout_add_widget(layout, label)
    qt::widget_set_layout(container, layout)

    qt::window_set_central_widget(window, container)
    qt::show(window)
    qt::run()          ;; blokująca pętla zdarzeń Qt
end
```

Zobacz `examples/counter_app.h#` po przykład z własną pętlą
(`poll_event()`), przyciskiem i polem tekstowym.

---

## API — przegląd

### Aplikacja
| Funkcja | Opis |
|---|---|
| `qt::init()` | Tworzy `QApplication` (jednorazowo, wywołaj na początku `main()`) |
| `qt::run() -> int` | Blokująca pętla zdarzeń Qt (`exec()`), zwraca kod wyjścia |
| `qt::quit()` | Kończy pętlę `run()` |
| `qt::process_events()` | Przetwarza oczekujące zdarzenia bez blokowania (dla własnej pętli) |
| `qt::is_running() -> bool` | Czy nikt nie wywołał jeszcze `quit()` |
| `qt::set_style(name: string)` | np. `"Fusion"` |
| `qt::set_app_name(name: string)` | Nazwa aplikacji |

### Tworzenie widgetów (zwracają `Widget`)
`qt::new_window(title, w, h)`, `qt::new_container()`, `qt::new_button(text)`,
`qt::new_label(text)`, `qt::new_lineedit(placeholder)`, `qt::new_checkbox(text)`,
`qt::new_slider(min, max, initial)`

### Layouty (zwracają `Layout`)
`qt::new_vbox()`, `qt::new_hbox()`, `qt::layout_add_widget(l, w)`,
`qt::layout_add_spacing(l, px)`, `qt::layout_add_stretch(l, stretch)`,
`qt::layout_set_margins(l, l, t, r, b)`, `qt::widget_set_layout(widget, layout)`

### Operacje ogólne na widgetach
`qt::show`, `qt::hide`, `qt::close`, `qt::resize`, `qt::move_to`,
`qt::set_enabled`, `qt::set_visible`, `qt::set_style_sheet` (CSS Qt),
`qt::set_fixed_size`, `qt::destroy`

### Specyficzne dla typu widgetu
- **Label:** `qt::label_set_text`, `qt::label_get_text`
- **Button:** `qt::button_set_text`
- **LineEdit:** `qt::lineedit_get_text`, `qt::lineedit_set_text`,
  `qt::lineedit_set_placeholder`, `qt::lineedit_set_read_only`,
  `qt::lineedit_set_password_mode`
- **CheckBox:** `qt::checkbox_is_checked`, `qt::checkbox_set_checked`
- **Slider:** `qt::slider_get_value`, `qt::slider_set_value`
- **Okno:** `qt::window_set_central_widget`, `qt::window_set_title`

### Dialogi (blokujące, natywne okna Qt)
`qt::show_info`, `qt::show_warning`, `qt::show_error`,
`qt::show_question(...) -> bool`

### Zdarzenia
```hsharp
struct Event is
    pub kind:   EventKind
    pub widget: Widget
end

enum EventKind is
    Clicked
    TextChanged
    ReturnPressed
    Toggled
    WindowClosed
    ValueChanged
    Unknown
end

pub fn poll_event() -> Event?
```

`ev.widget.handle` jest tym samym `int`, który dostałeś z odpowiedniego
`qt::new_*` — porównuj uchwyty, żeby rozróżnić, który widget wysłał
zdarzenie, gdy masz ich wiele.

---

## Znane ograniczenia

- **Brak callbacków przez FFI.** Jak opisano wyżej — zdarzenia idą przez
  kolejkę i `poll_event()`, nie przez bezpośrednie sygnały/sloty H#.
- **Uchwyty to surowe wskaźniki jako `int`.** Biblioteka nie liczy
  referencji ani nie waliduje uchwytów — nie wywołuj funkcji na uchwycie
  po `qt::destroy()` tego widgetu (dokładnie tak samo jak surowy
  `free()`/`malloc()` w przykładzie FFI z README H#).
- **Statyczne linkowanie Qt6 samego w sobie nie jest wspierane** przez
  dystrybucje Qt (Qt Widgets zwykle dystrybuowane jest jako `.so`).
  `native/libqthsharp.a` jest statyczny (zgodnie z życzeniem — `extern
  static`), ale sam Qt6 pod spodem i tak ładowany jest dynamicznie
  (`libQt6Widgets.so` itd.) — to normalna, wspierana konfiguracja.
- Struktury przekazywane przez wartość przez `extern` w H# mają
  ograniczenia (patrz komentarze w kompilatorze H# / `ffi.rs`) — dlatego
  ta biblioteka nie przekazuje żadnych structów H# do C, tylko proste
  typy (`int`, `i32`, `i8`, `string`).

---

## Licencja

MIT — zobacz [`LICENSE`](./LICENSE).
