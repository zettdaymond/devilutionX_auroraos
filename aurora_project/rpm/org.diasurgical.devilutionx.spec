Name:       org.diasurgical.devilutionx
Summary:    Diablo (1996) port for Aurora OS
Version:    1.5.5
Release:    1
License:    Sustainable Use License
URL:        https://github.com/diasurgical/devilutionX
Source0:    %{name}-%{version}.tar.bz2

Requires: libdbus-1.so.3
Requires: libglib-2.0.so.0
Requires: libaudioresource.so.1
Requires: libwayland-client.so.0
Requires: libz.so.1
Requires: libbz2.so.1

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

# For zoe build
BuildRequires: libcurl-devel


%description
Тьма зашевелилась в глубинах Тристрама. Древнее зло шагает по земле, разжигая гражданскую войну и вселяя ужас в народ.
Обезумевший король, его пропавший сын и загадочный архиепископ — это ключи к загадке, которую тебе предстоит разрешить.
Ты прибыл к источнику зла. Это город Тристрам, где теперь живет лишь горстка выживших, сломленных и искалеченных выпавшим на их долю безумием.
Здесь стоит собор, возведенный на руинах древнего монастыря. Жуткие огни и чудовищные звуки эхом разносятся по его заброшенным залам,
и именно туда предстоит отправиться тебе. Найди в себе смелость бросить вызов Изначальному Злу...

- Полнофункциональный порт Diablo и Hellfire для Аврора ОС (основа — DevilutionX 1.5.5)
- Встроенный лаунчер: загрузка файлов игры, настройки движка, выбор папки с данными
- Сенсорное управление и виртуальный геймпад
- Поворот экрана в обоих ландшафтных режимах, живая плитка с обложкой
- Честная пауза в свёрнутом состоянии (экономия батареи)
- Доступно на русском (текст и озвучка) и других языках
- Сотни исправлений ошибок оригинальной игры

Как установить полную версию:

1. Установите приложение DevilutionX и запустите его.
2. Бесплатную демо-версию и русскую озвучку (ru.mpq) можно скачать прямо в приложении — экран «Данные».
3. Для полной версии найдите DIABDAT.MPQ на компакт-диске, в папке установки или с помощью Innoextract,
   и перенесите его в папку ~/Documents/devilutionx/ (или выберите любую папку на экране «Данные»).
   Подключите устройство к компьютеру и разрешите доступ к данным, нажав «Использовать протокол передачи мультимедиа MTP».
4. Для запуска дополнения Diablo: Hellfire перенесите также hellfire.mpq, hfmonk.mpq, hfmusic.mpq и hfvoice.mpq.

Без файлов оригинальной игры доступна демонстрационная часть (shareware-версия от Blizzard);
полную версию можно купить на GoG.com.

%prep
%autosetup

%build
CXXFLAGS=-O0 %cmake
CXXFLAGS=-O0 %ninja_build

%install
%ninja_install

%files
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%{_datadir}/%{name}/assets/*
