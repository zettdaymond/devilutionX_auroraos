Name:       org.diasurgical.devilutionx
Summary:    Diablo (1996) port for Aurora OS
Version:    1.5.3
Release:    1
License:    Sustainable Use License
URL:        https://github.com/diasurgical/devilutionX
Source0:    %{name}-%{version}.tar.bz2

Requires: libQt5Core.so.5
Requires: libQt5Gui.so.5
Requires: libSDL2-2.0.so.0
Requires: libSDL2_image-2.0.so.0
Requires: libdbus-1.so.3
Requires: libglib-2.0.so.0
Requires: libaudioresource.so.1
Requires: libwayland-client.so.0
Requires: libz.so.1
Requires: libbz2.so.1

BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)

BuildRequires: gcc
BuildRequires: cmake
BuildRequires: ninja
BuildRequires: git

BuildRequires: wayland-devel

BuildRequires: zlib
BuildRequires: zlib-devel

BuildRequires: gettext

BuildRequires: bzip2
BuildRequires: bzip2-devel
BuildRequires: bzip2-libs

BuildRequires: libaudioresource
BuildRequires: libaudioresource-devel

# For SDL2 static build
BuildRequires: pkgconfig(wayland-egl)
BuildRequires: pkgconfig(wayland-client)
BuildRequires: pkgconfig(wayland-cursor)
BuildRequires: pkgconfig(wayland-protocols)
BuildRequires: pkgconfig(wayland-scanner)
BuildRequires: pkgconfig(egl)
BuildRequires: pkgconfig(glesv1_cm)
BuildRequires: pkgconfig(glesv2)
BuildRequires: pkgconfig(xkbcommon)
BuildRequires: pkgconfig(libpulse-simple)
BuildRequires: pulseaudio-devel


%description
Тьма зашевелилась в глубинах Тристрама. Древнее зло шагает по земле, разжигая гражданскую войну и вселяя ужас в народ.
Обезумевший король, его пропавший сын и загадочный архиепископ — это ключи к загадке, которую тебе предстоит разрешить.
Ты прибыл к источнику зла. Это город Тристрам, где теперь живет лишь горстка выживших, сломленных и искалеченных выпавшим на их долю безумием.
Здесь стоит собор, возведенный на руинах древнего монастыря. Жуткие огни и чудовищные звуки эхом разносятся по его заброшенным залам,
и именно туда предстоит отправиться тебе. Найди в себе смелость бросить вызов Изначальному Злу...

Примечание: вам нужно будет добавить данные из оригинала, чтобы играть в полную игру.
Если у вас нет оригинального компакт-диска, вы можете купить Diablo на GoG.com.
Без него у вас будет доступ только к демонстрационной части игры с shareware версии от Blizzard.

- Полнофункциональный порт Diablo для Аврора ОС
- Доступно на русском и других языках
- Множество тонких улучшений
- Сотни исправлений ошибок в оригинальной игре

Как установить полную версию:

1. Установите приложение DevilutionX.
2. Найдите DIABDAT.MPQ на компакт-диске, в папке установки или с помощью Innoextract.
3. Подключите устройство к компьютеру и разрешите доступ к данным нажав "Использовать протокол передачи мультимедиа MTP".
4. Перенесите DIABDAT.MPQ в папку ~/Documents/devilutionx/ ( или ~/.local/share/org.diasurgical/devilutionx/ )
5. Для русской озвучки можно скачать ru.mpq ( https://github.com/diasurgical/devilutionx-assets/releases/download/v4/ru.mpq )
   и перенести его в папку ~/Documents/devilutionx/ ( или ~/.local/share/org.diasurgical/devilutionx/ )
6. Для запуска дополнения Diablo: Hellfire нужно также перенести дополнительные файлы - hellfire.mpq, hfmonk.mpq, hfmusic.mpq, hfvoice.mpq

%prep
%autosetup

%build
%cmake
%ninja_build

%install
%ninja_install

%files
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%{_datadir}/%{name}/assets/*
