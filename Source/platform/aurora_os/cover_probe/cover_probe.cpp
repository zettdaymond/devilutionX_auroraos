// Зонд размера плитки обложки: единственная задача — напечатать Silica
// Theme.coverSizeVertical / coverSizeHorizontal и ориентацию обложки, и
// выйти. Lipstick 5.1 не сообщает размер обложки через wayland
// configure, его знает только Silica-тема; Qt-приложения читают Theme
// напрямую. Отдельный процесс — Qt и Silica-плагин не попадают в
// адресное пространство игры.
//
// Выход (stdout):
//   vertical <width> <height>
//   horizontal <width> <height>
//   coverOrientation <int>  — Cover.orientation (документированный
//                              сигнал Silica: Cover.Vertical/Cover.
//                              Horizontal — как обложка показывается).
//   native <int>            — QScreen::nativeOrientation (Portrait=1,
//                              Landscape=2, ...): врождённая ориентация
//                              устройства; запасной выбор пары, если
//                              Cover.orientation молчит (headless-риск).
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QScreen>
#include <QUrl>

#include <cstdio>

namespace {

constexpr const char *kQml = R"QML(
import QtQml 2.0
import Sailfish.Silica 1.0
CoverPlaceholder {
    property real vw: Theme.coverSizeVertical.width
    property real vh: Theme.coverSizeVertical.height
    property real hw: Theme.coverSizeHorizontal.width
    property real hh: Theme.coverSizeHorizontal.height
}
)QML";

} // namespace

int main(int argc, char *argv[])
{
	QGuiApplication app(argc, argv);
	QQmlEngine engine;
	QQmlComponent component(&engine);
	component.setData(kQml, QUrl("file:///devilutionx-coverprobe.qml"));
	if (!component.isReady()) {
		std::fprintf(stderr, "devilutionx-coverprobe: QML не собрался: %s\n",
		    qPrintable(component.errorString()));
		return 1;
	}
	QObject *object = component.create();
	if (object == nullptr) {
		std::fprintf(stderr, "devilutionx-coverprobe: Theme не создан\n");
		return 1;
	}
	std::printf("vertical %.0f %.0f\nhorizontal %.0f %.0f\n",
	    object->property("vw").toDouble(), object->property("vh").toDouble(),
	    object->property("hw").toDouble(), object->property("hh").toDouble());
	// orientation может быть неопределена, пока Cover не показан
	// композитором: QVariant() печатается как пустая строка.
	const QVariant orientation = object->property("orientation");
	if (orientation.isValid()) {
		std::printf("coverOrientation %d\n", orientation.toInt());
	}
	std::printf("native %d\n",
	    static_cast<int>(QGuiApplication::primaryScreen()->nativeOrientation()));
	delete object;
	return 0;
}
