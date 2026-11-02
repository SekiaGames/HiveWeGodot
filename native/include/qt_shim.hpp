#pragma once

// Qt 的最小替身：只实现核心层（base / brush / file_formats / resources）用到的那几个类。
// 用 -I native/include 让 `#include <QRect>` 这类原始写法能直接找到同名转发头。

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <string>
#include <string_view>
#include <cstdio>
#include <vector>

class QString {
  public:
	QString() = default;
	QString(const char* value): value_(value) {}
	QString(std::string value): value_(std::move(value)) {}
	QString(std::string_view value): value_(value) {}
	QString(char value): value_(1, value) {}

	static QString fromStdString(const std::string& value) { return QString(value); }
	static QString fromUtf8(const char* value, int length = -1) {
		return QString(length < 0 ? std::string(value) : std::string(value, static_cast<std::size_t>(length)));
	}
	static QString fromUtf8(std::string_view value) { return QString(value); }
	static QString fromStdString(std::string_view value) { return QString(value); }

	std::string toStdString() const { return this->value_; }
	const std::string& str() const { return this->value_; }
	const char* c_str() const { return this->value_.c_str(); }
	int toInt() const { return std::atoi(this->value_.c_str()); }
	bool toBool() const { return this->value_ == "true" || this->value_ == "True" || this->value_ == "1"; }
	bool isEmpty() const { return this->value_.empty(); }
	std::size_t size() const { return this->value_.size(); }

	bool contains(const QString& needle) const { return this->value_.find(needle.value_) != std::string::npos; }
	bool contains(const char* needle) const { return this->value_.find(needle) != std::string::npos; }

	int indexOf(const QString& needle) const {
		const std::size_t pos = this->value_.find(needle.value_);
		return pos == std::string::npos ? -1 : static_cast<int>(pos);
	}

	QString mid(int start) const {
		if (start < 0 || static_cast<std::size_t>(start) >= this->value_.size()) {
			return QString();
		}
		return QString(this->value_.substr(start));
	}

	QString& replace(const QString& from, const QString& to) {
		if (from.value_.empty()) {
			return *this;
		}
		std::size_t pos = 0;
		while ((pos = this->value_.find(from.value_, pos)) != std::string::npos) {
			this->value_.replace(pos, from.value_.size(), to.value_);
			pos += to.value_.size();
		}
		return *this;
	}

	QString& append(const QString& value) {
		this->value_ += value.value_;
		return *this;
	}

	QString arg(const QString& value) const {
		std::size_t best_pos = std::string::npos;
		int best_num = 100;
		std::size_t best_len = 0;

		for (std::size_t i = 0; i + 1 < this->value_.size(); ++i) {
			if (this->value_[i] != '%' || !std::isdigit(static_cast<unsigned char>(this->value_[i + 1]))) {
				continue;
			}

			std::size_t j = i + 1;
			int number = 0;
			while (j < this->value_.size() && std::isdigit(static_cast<unsigned char>(this->value_[j])) && j - i <= 2) {
				number = number * 10 + (this->value_[j] - '0');
				++j;
			}

			if (number < best_num) {
				best_num = number;
				best_pos = i;
				best_len = j - i;
			}
		}

		if (best_pos == std::string::npos) {
			return *this;
		}

		std::string result = this->value_.substr(0, best_pos) + value.value_ + this->value_.substr(best_pos + best_len);
		return QString(result);
	}

	QString arg(const char* value) const { return this->arg(QString(value)); }
	QString arg(int value) const { return this->arg(QString(std::to_string(value))); }
	QString arg(std::size_t value) const { return this->arg(QString(std::to_string(value))); }

	QString& operator+=(const QString& other) {
		this->value_ += other.value_;
		return *this;
	}

	friend bool operator==(const QString& a, const QString& b) { return a.value_ == b.value_; }
	friend bool operator!=(const QString& a, const QString& b) { return a.value_ != b.value_; }
	friend bool operator==(const QString& a, const char* b) { return a.value_ == b; }
	friend bool operator!=(const QString& a, const char* b) { return a.value_ != b; }

  private:
	std::string value_;
};

class QStringList: public std::vector<QString> {
  public:
	using std::vector<QString>::vector;

	void append(const QString& value) { this->push_back(value); }

	QStringList& operator+=(const QString& value) {
		this->push_back(value);
		return *this;
	}
	void append(const char* value) { this->push_back(QString(value)); }

	QString join(const QString& separator) const {
		std::string result;
		for (std::size_t i = 0; i < this->size(); ++i) {
			if (i != 0) {
				result += separator.str();
			}
			result += (*this)[i].str();
		}
		return QString(result);
	}
};

inline QString operator+(const QString& lhs, const QString& rhs) { return QString(lhs.str() + rhs.str()); }
inline QString operator+(const QString& lhs, const char* rhs) { return QString(lhs.str() + rhs); }
inline QString operator+(const char* lhs, const QString& rhs) { return QString(std::string(lhs) + rhs.str()); }

#define Q_OBJECT
#define emit

class QImage {
  public:
	enum class Format {
		Format_RGB888,
		Format_RGBA8888,
		Format_ARGB32,
		Format_Grayscale8,
	};

	QImage() = default;
	QImage(const unsigned char*, int width, int height, Format): w_(width), h_(height) {}

	int width() const { return this->w_; }
	int height() const { return this->h_; }

  private:
	int w_ = 0;
	int h_ = 0;
};

class QPixmap {
  public:
	QPixmap() = default;
	static QPixmap fromImage(const QImage&) { return QPixmap(); }
};

class QIcon {
  public:
	QIcon() = default;
	explicit QIcon(const QPixmap&) {}
};

class QVariant {
  public:
	QVariant() = default;
	QVariant(int value): value_(std::to_string(value)) {}
	QVariant(bool value): value_(value ? "1" : "0") {}
	QVariant(const char* value): value_(value) {}
	QVariant(const QString& value): value_(value.str()) {}
	QVariant(const QIcon&): value_() {}

	int toInt() const { return std::atoi(this->value_.c_str()); }
	bool toBool() const { return this->value_ == "1" || this->value_ == "true"; }
	QString toString() const { return QString(this->value_); }

  private:
	std::string value_;
};

class QSettings {
  public:
	enum Format { NativeFormat, IniFormat };

	QSettings() = default;
	QSettings(const QString&, Format = NativeFormat) {}

	bool contains(const QString&) const { return false; }
	QVariant value(const QString&, const QVariant& fallback = QVariant()) const { return fallback; }
	void setValue(const QString&, const QVariant&) {}
	void sync() {}
};

class QObject {
  public:
	QObject() = default;
	virtual ~QObject() = default;
};

class QDir {
  public:
	static QString tempPath() { return QString("/tmp"); }
};

class QProcess {
  public:
	void setWorkingDirectory(const QString& directory) { this->directory_ = directory; }

	void start(const QString& program, const QStringList& arguments) {
		std::string command = "cd \"" + this->directory_.str() + "\" 2>/dev/null && " + program.str();
		for (const QString& argument: arguments) {
			command += " \"" + argument.str() + "\"";
		}
		command += " 2>&1";

		FILE* pipe = popen(command.c_str(), "r");
		if (!pipe) {
			return;
		}

		char buffer[4096];
		while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
			this->output_ += buffer;
		}
		pclose(pipe);
	}

	void waitForFinished() {}

	QString readAllStandardOutput() const { return QString(this->output_); }

  private:
	QString directory_;
	std::string output_;
};

class QWidget;

class QMessageBox {
  public:
	enum class StandardButton {
		Ok = 0x400,
		Cancel = 0x400000,
		Yes = 0x4000,
		No = 0x10000,
	};

	static void critical(QWidget*, const QString& title, const QString& text, StandardButton = StandardButton::Ok) {
		report("critical", title, text);
	}
	static void warning(QWidget*, const QString& title, const QString& text, StandardButton = StandardButton::Ok) {
		report("warning", title, text);
	}
	static void information(QWidget*, const QString& title, const QString& text, StandardButton = StandardButton::Ok) {
		report("information", title, text);
	}

	QMessageBox() = default;
	void setText(const QString& text) { this->text_ = text; }
	void exec() { report("message", QString(""), this->text_); }

  private:
	QString text_;

	static void report(const char* level, const QString& title, const QString& text) {
		std::fprintf(stderr, "[%s] %s: %s\n", level, title.c_str(), text.c_str());
	}
};

class QWidget {
  public:
	QWidget() = default;
	virtual ~QWidget() = default;
};

class QRect {
  public:
	QRect() = default;
	QRect(int x, int y, int w, int h): x_(x), y_(y), w_(w), h_(h) {}

	int x() const { return this->x_; }
	int y() const { return this->y_; }
	int width() const { return this->w_; }
	int height() const { return this->h_; }
	int left() const { return this->x_; }
	int top() const { return this->y_; }
	int right() const { return this->x_ + this->w_ - 1; }
	int bottom() const { return this->y_ + this->h_ - 1; }

	bool isEmpty() const { return this->w_ <= 0 || this->h_ <= 0; }

	bool contains(int px, int py) const {
		return px >= this->left() && px <= this->right() && py >= this->top() && py <= this->bottom();
	}

	void setX(int value) { this->x_ = value; }
	void setY(int value) { this->y_ = value; }
	void setLeft(int value) { this->w_ += this->x_ - value; this->x_ = value; }
	void setTop(int value) { this->h_ += this->y_ - value; this->y_ = value; }
	void setRight(int value) { this->w_ = value - this->x_ + 1; }
	void setBottom(int value) { this->h_ = value - this->y_ + 1; }
	void setWidth(int value) { this->w_ = value; }
	void setHeight(int value) { this->h_ = value; }

	QRect& operator|=(const QRect& other) {
		*this = this->united(other);
		return *this;
	}

	QRect& operator&=(const QRect& other) {
		*this = this->intersected(other);
		return *this;
	}

	QRect adjusted(int dx1, int dy1, int dx2, int dy2) const {
		return QRect(this->x_ + dx1, this->y_ + dy1, this->w_ + dx2 - dx1, this->h_ + dy2 - dy1);
	}

	QRect intersected(const QRect& other) const {
		if (this->isEmpty() || other.isEmpty()) {
			return QRect();
		}
		const int l = std::max(this->left(), other.left());
		const int t = std::max(this->top(), other.top());
		const int r = std::min(this->right(), other.right());
		const int b = std::min(this->bottom(), other.bottom());
		if (r < l || b < t) {
			return QRect();
		}
		return QRect(l, t, r - l + 1, b - t + 1);
	}

	QRect united(const QRect& other) const {
		if (this->isEmpty()) {
			return other;
		}
		if (other.isEmpty()) {
			return *this;
		}
		const int l = std::min(this->left(), other.left());
		const int t = std::min(this->top(), other.top());
		const int r = std::max(this->right(), other.right());
		const int b = std::max(this->bottom(), other.bottom());
		return QRect(l, t, r - l + 1, b - t + 1);
	}

	bool operator==(const QRect& other) const {
		return this->x_ == other.x_ && this->y_ == other.y_ && this->w_ == other.w_ && this->h_ == other.h_;
	}

  private:
	int x_ = 0;
	int y_ = 0;
	int w_ = 0;
	int h_ = 0;
};

class QRectF {
  public:
	QRectF() = default;
	QRectF(double x, double y, double w, double h): x_(x), y_(y), w_(w), h_(h) {}
	QRectF(const QRect& r): x_(r.x()), y_(r.y()), w_(r.width()), h_(r.height()) {}

	double x() const { return this->x_; }
	double y() const { return this->y_; }
	double width() const { return this->w_; }
	double height() const { return this->h_; }
	double left() const { return this->x_; }
	double top() const { return this->y_; }
	double right() const { return this->x_ + this->w_; }
	double bottom() const { return this->y_ + this->h_; }

	bool isEmpty() const { return this->w_ <= 0.0 || this->h_ <= 0.0; }

	QRectF adjusted(double dx1, double dy1, double dx2, double dy2) const {
		return QRectF(this->x_ + dx1, this->y_ + dy1, this->w_ + dx2 - dx1, this->h_ + dy2 - dy1);
	}

	bool contains(double px, double py) const {
		return px >= this->left() && px <= this->right() && py >= this->top() && py <= this->bottom();
	}

	bool contains(const QRectF& other) const {
		return other.left() >= this->left() && other.right() <= this->right()
			&& other.top() >= this->top() && other.bottom() <= this->bottom();
	}

	QRectF united(const QRectF& other) const {
		if (this->isEmpty()) {
			return other;
		}
		if (other.isEmpty()) {
			return *this;
		}
		const double l = std::min(this->left(), other.left());
		const double t = std::min(this->top(), other.top());
		const double r = std::max(this->right(), other.right());
		const double b = std::max(this->bottom(), other.bottom());
		return QRectF(l, t, r - l, b - t);
	}

	QRectF intersected(const QRectF& other) const {
		const double l = std::max(this->left(), other.left());
		const double t = std::max(this->top(), other.top());
		const double r = std::min(this->right(), other.right());
		const double b = std::min(this->bottom(), other.bottom());
		if (r <= l || b <= t) {
			return QRectF();
		}
		return QRectF(l, t, r - l, b - t);
	}

	QRect toRect() const {
		return QRect(static_cast<int>(std::lround(this->x_)), static_cast<int>(std::lround(this->y_)),
					 static_cast<int>(std::lround(this->w_)), static_cast<int>(std::lround(this->h_)));
	}

  private:
	double x_ = 0.0;
	double y_ = 0.0;
	double w_ = 0.0;
	double h_ = 0.0;
};

namespace Qt {
enum MouseButton {
	NoButton = 0x0,
	LeftButton = 0x1,
	RightButton = 0x2,
	MiddleButton = 0x4,
};

enum KeyboardModifier {
	NoModifier = 0x0,
	ShiftModifier = 0x02000000,
	ControlModifier = 0x04000000,
	AltModifier = 0x08000000,
	KeypadModifier = 0x20000000,
	AnyModifier = 0xffffffff,
};

enum ItemFlag {
	NoItemFlags = 0x0,
	ItemIsSelectable = 0x1,
	ItemIsEditable = 0x2,
	ItemIsDragEnabled = 0x4,
	ItemIsDropEnabled = 0x8,
	ItemIsUserCheckable = 0x10,
	ItemIsEnabled = 0x20,
};

enum ItemDataRole {
	DisplayRole = 0,
	DecorationRole = 1,
	EditRole = 2,
	ToolTipRole = 3,
	ForegroundRole = 9,
	CheckStateRole = 10,
	UserRole = 0x0100,
};

enum CheckState {
	Unchecked = 0,
	PartiallyChecked = 1,
	Checked = 2,
};

enum Orientation {
	Horizontal = 0x1,
	Vertical = 0x2,
};

enum DropAction {
	CopyAction = 0x1,
	MoveAction = 0x2,
	IgnoreAction = 0x0,
};

using ItemFlags = unsigned int;
using DropActions = unsigned int;

enum ConnectionType {
	AutoConnection = 0,
	DirectConnection = 1,
	QueuedConnection = 2,
};

enum Key {
	Key_Escape = 0x01000000,
	Key_Tab = 0x01000001,
	Key_Backspace = 0x01000003,
	Key_Return = 0x01000004,
	Key_Enter = 0x01000005,
	Key_Insert = 0x01000006,
	Key_Delete = 0x01000007,
	Key_Home = 0x01000010,
	Key_End = 0x01000011,
	Key_Left = 0x01000012,
	Key_Up = 0x01000013,
	Key_Right = 0x01000014,
	Key_Down = 0x01000015,
	Key_PageUp = 0x01000016,
	Key_PageDown = 0x01000017,
	Key_Shift = 0x01000020,
	Key_Control = 0x01000021,
	Key_Meta = 0x01000022,
	Key_Alt = 0x01000023,
	Key_F1 = 0x01000030,
	Key_F2 = 0x01000031,
	Key_F3 = 0x01000032,
	Key_F4 = 0x01000033,
	Key_F5 = 0x01000034,
	Key_Space = 0x20,
	Key_0 = 0x30,
	Key_1 = 0x31,
	Key_2 = 0x32,
	Key_3 = 0x33,
	Key_4 = 0x34,
	Key_5 = 0x35,
	Key_6 = 0x36,
	Key_7 = 0x37,
	Key_8 = 0x38,
	Key_9 = 0x39,
	Key_Minus = 0x2d,
	Key_Equal = 0x3d,
	Key_A = 0x41,
	Key_B = 0x42,
	Key_C = 0x43,
	Key_D = 0x44,
	Key_E = 0x45,
	Key_F = 0x46,
	Key_G = 0x47,
	Key_H = 0x48,
	Key_I = 0x49,
	Key_J = 0x4a,
	Key_K = 0x4b,
	Key_L = 0x4c,
	Key_M = 0x4d,
	Key_N = 0x4e,
	Key_O = 0x4f,
	Key_P = 0x50,
	Key_Q = 0x51,
	Key_R = 0x52,
	Key_S = 0x53,
	Key_T = 0x54,
	Key_U = 0x55,
	Key_V = 0x56,
	Key_W = 0x57,
	Key_X = 0x58,
	Key_Y = 0x59,
	Key_Z = 0x5a,
};
}

class QPoint {
  public:
	QPoint() = default;
	QPoint(int x, int y): x_(x), y_(y) {}

	int x() const { return this->x_; }
	int y() const { return this->y_; }

  private:
	int x_ = 0;
	int y_ = 0;
};

class QMouseEvent {
  public:
	QMouseEvent() = default;
	QMouseEvent(QPoint pos, Qt::MouseButton button, Qt::MouseButton buttons, Qt::KeyboardModifier modifiers = Qt::NoModifier):
		pos_(pos), button_(button), buttons_(buttons), modifiers_(modifiers) {}

	QPoint pos() const { return this->pos_; }
	Qt::MouseButton button() const { return this->button_; }
	Qt::MouseButton buttons() const { return this->buttons_; }
	Qt::KeyboardModifier modifiers() const { return this->modifiers_; }

  private:
	QPoint pos_{};
	Qt::MouseButton button_ = Qt::NoButton;
	Qt::MouseButton buttons_ = Qt::NoButton;
	Qt::KeyboardModifier modifiers_ = Qt::NoModifier;
};

class QWheelEvent {
  public:
	QWheelEvent() = default;
	explicit QWheelEvent(int delta): delta_(0, delta) {}

	QPoint angleDelta() const { return this->delta_; }

  private:
	QPoint delta_{};
};

class QKeyEvent {
  public:
	QKeyEvent() = default;
	QKeyEvent(Qt::Key key, Qt::KeyboardModifier modifiers = Qt::NoModifier): key_(key), modifiers_(modifiers) {}

	Qt::Key key() const { return this->key_; }
	Qt::KeyboardModifier modifiers() const { return this->modifiers_; }
	bool isAutoRepeat() const { return false; }

  private:
	Qt::Key key_ = Qt::Key_Escape;
	Qt::KeyboardModifier modifiers_ = Qt::NoModifier;
};

template <class T>
using QVector = std::vector<T>;

template <class Sender, class Signal, class Slot>
void connect(Sender*, Signal, Slot) {}

class QModelIndex {
  public:
	QModelIndex() = default;
	QModelIndex(int row, int column): row_(row), column_(column) {}

	int row() const { return this->row_; }
	int column() const { return this->column_; }
	QModelIndex parent() const { return QModelIndex(); }
	QModelIndex sibling(int, int) const { return QModelIndex(); }
	bool isValid() const { return this->row_ >= 0 && this->column_ >= 0; }

  private:
	int row_ = -1;
	int column_ = -1;
};

class QAbstractTableModel {
  public:
	explicit QAbstractTableModel(QObject* = nullptr) {}
	virtual ~QAbstractTableModel() = default;

	virtual int rowCount(const QModelIndex& = QModelIndex()) const { return 0; }
	virtual int columnCount(const QModelIndex& = QModelIndex()) const { return 0; }
	virtual QVariant data(const QModelIndex&, int = 0) const { return QVariant(); }
	virtual bool setData(const QModelIndex&, const QVariant&, int = 0) { return false; }
	virtual QVariant headerData(int, Qt::Orientation, int = 0) const { return QVariant(); }
	virtual Qt::ItemFlags flags(const QModelIndex&) const { return Qt::NoItemFlags; }

	void dataChanged(const QModelIndex& = QModelIndex(), const QModelIndex& = QModelIndex(), const std::vector<int>& = {}) {}
	void rowsAboutToBeRemoved(const QModelIndex& = QModelIndex(), int = 0, int = 0) {}
	void rowsRemoved(const QModelIndex& = QModelIndex(), int = 0, int = 0) {}
	void rowsAboutToBeInserted(const QModelIndex& = QModelIndex(), int = 0, int = 0) {}
	void rowsInserted(const QModelIndex& = QModelIndex(), int = 0, int = 0) {}
	void columnsAboutToBeRemoved(const QModelIndex& = QModelIndex(), int = 0, int = 0) {}
	void columnsRemoved(const QModelIndex& = QModelIndex(), int = 0, int = 0) {}
	void columnsAboutToBeInserted(const QModelIndex& = QModelIndex(), int = 0, int = 0) {}
	void columnsInserted(const QModelIndex& = QModelIndex(), int = 0, int = 0) {}
	void modelAboutToBeReset() {}
	void modelReset() {}
	void layoutAboutToBeChanged() {}
	void layoutChanged() {}

  protected:
	void headerDataChanged(Qt::Orientation, int, int) {}
	QModelIndex createIndex(int row, int column, void* = nullptr) const { return QModelIndex(row, column); }

	void beginResetModel() {}
	void endResetModel() {}
	void beginInsertRows(const QModelIndex&, int, int) {}
	void endInsertRows() {}
	void beginRemoveRows(const QModelIndex&, int, int) {}
	void endRemoveRows() {}
	void beginMoveRows(const QModelIndex&, int, int, const QModelIndex&, int) {}
	void endMoveRows() {}
	void beginFilterChange() {}
	void endFilterChange() {}
	void beginInsertColumns(const QModelIndex&, int, int) {}
	void endInsertColumns() {}
	void beginRemoveColumns(const QModelIndex&, int, int) {}
	void endRemoveColumns() {}
};

class QAbstractItemModel;

class QAbstractProxyModel: public QAbstractTableModel {
  public:
	explicit QAbstractProxyModel(QObject* parent = nullptr): QAbstractTableModel(parent) {}

	virtual QModelIndex mapFromSource(const QModelIndex&) const { return QModelIndex(); }
	virtual QModelIndex mapToSource(const QModelIndex&) const { return QModelIndex(); }
	virtual QModelIndex index(int row, int column, const QModelIndex& = QModelIndex()) const { return QModelIndex(row, column); }
	virtual QModelIndex parent(const QModelIndex&) const { return QModelIndex(); }
	virtual void setSourceModel(QAbstractItemModel*) {}
};

