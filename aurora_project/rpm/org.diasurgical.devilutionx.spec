%undefine _auto_set_build_flags

Name:       org.diasurgical.devilutionx
Summary:    Diablo (1996) port for Aurora OS
Version:    1.6.0
Release:    1
License:    Sustainable Use License
URL:        https://github.com/diasurgical/devilutionX
Source0:    %{name}-%{version}.tar.bz2

Requires:   sailfishsilica-qt5 >= 0.10.9

BuildRequires: cmake
BuildRequires: ninja

BuildRequires: SDL2
BuildRequires: SDL2-devel

BuildRequires: SDL2_image
BuildRequires: SDL2_image-devel

BuildRequires: wayland-devel

BuildRequires: zlib
BuildRequires: zlib-devel

BuildRequires: gettext

BuildRequires: bzip2
BuildRequires: bzip2-devel
BuildRequires: bzip2-libs

BuildRequires: libaudioresource
BuildRequires: libaudioresource-devel


%description
Тьма зашевелилась в глубинах Тристрама. Древнее зло шагает по земле, разжигая гражданскую войну и вселяя ужас в народ.
Обезумевший король, его пропавший сын и загадочный архиепископ — это ключи к загадке, которую тебе предстоит разрешить.
Ты прибыл к источнику зла. Это город Тристрам, где теперь живет лишь горстка выживших, сломленных и искалеченных выпавшим на их долю безумием.
Здесь стоит собор, возведенный на руинах древнего монастыря. Жуткие огни и чудовищные звуки эхом разносятся по его заброшенным залам,
и именно туда предстоит отправиться тебе. Найди в себе смелость бросить вызов Изначальному Злу...

Примечание: вам нужно будет добавить данные из оригинала, чтобы играть в полную игру.
Если у вас нет оригинального компакт-диска, вы можете купить Diablo на GoG.com.
Без него у вас будет доступ только к демонстрационной части игры с shareware версии от Blizzard.

- Полнофункциональный порт Diablo для Аврора ОС.
- Доступно на русском и других языках
- Множество тонких улучшений
- Сотни исправлений ошибок в оригинальной игре.

Как установить полную версию:

1. Установите приложение DevilutionX.
2. Найдите DIABDAT.MPQ на компакт-диске, в папке установки или с помощью Innoextract.
3. Подключите устройство к компьютеру и разрешите доступ к данным нажав "Использовать протокол передачи мультимедиа MTP".
4. Перенесите DIABDAT.MPQ в папку Documents/devilutionx/
5. Для русской озвучки можно скачать ru.mpq ( https://github.com/diasurgical/devilutionx-assets/releases/download/v4/ru.mpq )
   и перенести его в папку Documents/devilutionx/
6. Для запуска дополнения Diablo: Hellfire нужно также перенести дополнительные файлы - hellfire.mpq, hfmonk.mpq, hfmusic.mpq, hfvoice.mpq

%prep
%autosetup

%build
%cmake -GNinja
%ninja_build

%install
%ninja_install

%files
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%{_datadir}/%{name}/assets/*
