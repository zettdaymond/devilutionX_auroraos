# Research: нативная обложка плитки Lipstick (Aurora OS 5.2)

Исследование механизма обложек домашнего экрана Авроры для не-Qt приложения
(DevilutionX, SDL2). Проведено 2026-09-05 на эмуляторе AuroraOS 5.2.1.200
(x86_64). Результат: рабочая нативная обложка, паритет с системными
приложениями — коммит `c9d5b6980` (WindowSystem WIP 3).

## Зачем

До этого плитка показывала последний буфер главного окна: наш код детектил
«мы свёрнуты» (стейтмашина на фокус/дисплей), переключал wl_buffer transform,
рисовал карточку в буфер. Нативный механизм отдает плитку композитору:
приложение держит отдельное окно-обложку, Lipstick сам показывает его в
нужный момент. Минус целый класс проблем (детект сворачивания, трансформы,
прозрачность за локскрином, GLES-смешение).

## Как устроена обложка в Aurora 5.2 (карта потребителя)

Читается сверху вниз — кто кого вызывает:

```
Silica-приложение (Qt)
  ├─ CoverWindow.qml (private) → Private.CoverWindow (C++, libsailfishsilicaplugin)
  │     — окно обложки, размер Theme.coverSize, title "_CoverWindow"
  │     — ставит "__winref:<N>" на главное окно (строка в .so)
  └─ QtWayland (libQt5WaylandClient)
        — qt_surface_extension → qt_extended_surface.update_generic_property

lipstick (композитор, liblipstick-qt5)
  ├─ серверная сторона qt_surface_extension: свойства → QWaylandSurface::windowProperties
  ├─ LipstickCompositor::windowIdForLink(siblingId, link)  [липпstickcompositor.cpp:389]
  │     — ищет окно ТОГО ЖЕ ПРОЦЕССА, чьё свойство WINID == link (uint)
  └─ LipstickCompositor::windowForId(id) — внутренние id окон

aurora-home (QML домашнего экрана, /usr/share/aurora-home/)
  ├─ WindowWrapperBase.qml — WindowProperty(SAILFISH_COVER_WINDOW) на главном окне
  │     — "__winref:N" разворачивается в coverId (windowproperty.cpp:99-106)
  ├─ WindowWrapper.qml:20 — coverHint = windowProperty("SAILFISH_HAVE_COVER")
  └─ Switcher.qml (ОТДЕЛЬНЫЙ процесс ru.auroraos.homescreen.switcher)
        — делегаты по foreign-toplevel-хэндлам; cover = windowForId(windowWrapper.coverId)
        — cover.resize(coverSize): свитчер РЕСАЙЗИТ окно обложки (configure!)
```

Ключевой вывод: линковка — по WINID-свойству обложки (uint, любое число),
привязанному строкой `__winref:<N>` на главном окне; вся трасса реактивная
(свойства обновляются на лету).

Референсы: auroraos-rs/aurora-gui (объявление механизма), форк
lmaxyz/winit@rm_maliut (рецепт set_transient для 5.2), sailfishos/lipstick
(исходники windowForLink/windowIdForLink).

## Хронология грабель (главное!)

Каждый пункт стоил цикла сборки — не повторять:

1. **UAF registry**: `wl_registry_bind` после `wl_registry_destroy` —
   краш. Биндить ДО destroy (забинженный прокси переживает смерть registry).
2. **Маршалинг new_id**: `wl_proxy_marshal_constructor` требует
   NULL-заглушку в слоте new_id, фактические аргументы — ПОСЛЕ неё.
   Без этого libwayland молча отвергает запрос («null value passed for
   arg 1»), свойства не уходят.
3. **set_transient к себе vs к главному**: приём автора winit-форка
   (`set_transient(own_surface)`) на 5.2.1.200 не сработал; рабочий
   вариант — transient к ГЛАВНОМУ окну (как у Qt). Вырожденный родитель,
   похоже, мешает lipstick создать оконный айтем вовсе.
4. **Роль окна обложки**: toplevel → поднимается отдельным окном;
   transient к главному + CATEGORY=cover ДО первого коммита → окно уходит
   в hiddenItem (JollaHomeLayout.qml:2467). Свойства ПОСЛЕ мапа не
   перекатегоризируют окно.
5. **Второй qt_extended_surface игнорируется**: свойства главного окна,
   отправленные через собственный extended surface (не созданный SDL),
   lipstick отбрасывает. Доказано зондом STATUSBAR_VISIBLE (статусбар
   появляется только через SDL-шный объект). Решение — патч SDL-форка:
   `3rdParty/SDL2/sdl-wayland-generic-property.patch` экспортирует
   `SDL_WaylandSetWindowGenericProperty` (использует wind->extended_surface).
6. **Fullscreen маскирует windowProperties**: на set_fullscreen-окне
   даже правильно доставленные свойства не действуют (тот же
   статусбар-зонд). Лаунчер временно создаётся оконным 720x1600;
   вернуть fullscreen можно только после ревизии на реальном устройстве.
7. **XRGB8888 → «спектр»**: буфер рендерился поканально разложенным
   (три изображения подряд). ARGB8888 — чисто.
8. **Пересоздание SHM-пулов → мусор**: уничтожение пула предыдущего
   кадра, пока композитор держит буфер, даёт полосы из переработанной
   памяти. Решение: один постоянный пул (memfd + mmap), кадры пишутся
   в ту же память, коммитится тот же wl_buffer.
9. **configure обязателен**: свитчер ресайзит окно обложки под карточку
   (cover.resize) — без ответа на configure плитка пуста. Контент —
   **aspect crop** (full-bleed), не fit (давало поля).
10. **Бета-эмулятора**: обложки в обычной плитке бывают выключены
    системно (у ВСЕХ приложений, включая Settings) — индикатор паритета:
    режим закрытия. Чинится перезапуском домашнего экрана.

## Итоговый рецепт (как в NativeCover.cpp)

```
1. wl_compositor.create_surface
2. wl_shell.get_shell_surface
3. set_transient(главное_окно, 0, 0, 0); set_title("cover"); set_class(<как у main>)
4. свойства (QVariant, BE; до первого коммита):
     main (через SDL-патч!):  WINID=1, SAILFISH_HAVE_COVER=true,
                              SAILFISH_COVER_WINDOW="__winref:2"
     cover:                    WINID=2, CATEGORY="cover", TRANSPARENT=false
5. SHM: memfd → wl_shm_create_pool (жить вечно) → create_buffer(ARGB8888)
6. RGB24 → аспект-кроп → mmap; attach + damage + commit
7. listener: ping → pong (обязательно!), configure → ресайз пула + перезаливка
```

QVariant-байты: `[u32 тип BE][байт isNull][payload BE]`; типы: bool=1,
uint=3, u64=5, string=10 (UTF-16BE, длина в байтах). Совпадает байт-в-байт
с aurora-gui (aurora_app/src/q_variant_compat.rs) и Qt.

## Инструменты отладки

- `WAYLAND_DEBUG=1` — полный wayland-трейс; эталон: `jolla-settings`
  с тем же env (его окно обложки создаётся лениво, при первом
  сворачивании — видно весь рабочий диалог).
- Зонды в билде (env):
  - `DEVILUTIONX_NATIVE_COVER_DEBUG=1` — сплошной красный буфер +
    кардиограмма (перезаливка каждые 500 мс);
  - `DEVILUTIONX_NATIVE_COVER_PROBE=1` — STATUSBAR_VISIBLE=true на
    главное окно (жив ли канал свойств);
  - `DEVILUTIONX_COVER_SEQ=a|b|c` — порядок роль/свойства/мап;
  - `DEVILUTIONX_NATIVE_COVER=0` — выключить нативную обложку;
  - `DEVILUTIONX_COVER_WINDOWED=0` — вернуть fullscreen.
- Лог: `aurora-native-cover:` в launcher.log.
- root на эмуляторе: `echo defaultuser | devel-su -c '<cmd>'` (НЕ -p).

## Открытые вопросы (WIP 4)

- Живой рендер карточки: ImGui RenderCover каждый кадр → UpdateFrame
  (прогресс загрузки в плитке); сейчас кадр запекается один раз.
- Кроп смещает текст карточки вверх — нужен якорь контента при кропе.
- Fullscreen главного окна: найти, как совмещается с windowProperties
  (возможно, APPLICATION_DISPLAY_MODE / роль windows changed?), вернуть
  полноэкранный лаунчер на устройстве.
- Реальное устройство: повторить проверку (эмулятор — бета-дом).
- Отладочные env-гейты выпилить перед релизом.
