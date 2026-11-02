#pragma once

// Qt 的最小替身：只实现核心层（base / brush / file_formats / resources）用到的那几个类。
// 用 -I native/include 让 `#include <QRect>` 这类原始写法能直接找到同名转发头。

#include <algorithm>
#include <cmath>

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

enum Key {
	Key_Escape = 0x01000000,
	Key_Delete = 0x01000007,
	Key_Left = 0x01000012,
	Key_Up = 0x01000013,
	Key_Right = 0x01000014,
	Key_Down = 0x01000015,
	Key_PageUp = 0x01000016,
	Key_PageDown = 0x01000017,
	Key_Minus = 0x2d,
	Key_Equal = 0x3d,
	Key_A = 0x41,
	Key_C = 0x43,
	Key_V = 0x56,
	Key_X = 0x58,
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
