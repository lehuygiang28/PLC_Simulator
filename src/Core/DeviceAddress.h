#ifndef CORE_DEVICEADDRESS_H
#define CORE_DEVICEADDRESS_H

#include <QString>

enum class DeviceKind { D, M };

enum class ValueView { Int16, Int32, Float, Double, String, Bit };

struct DeviceAddress
{
    DeviceKind kind = DeviceKind::D;
    int index = 0;  // D word number or M bit number
    int bit = -1;   // 0..15 if this is a D-bit; -1 otherwise

    static constexpr int kMaxIndex = 99999;

    bool isBit() const { return kind == DeviceKind::M || bit >= 0; }
    QString toString() const;
    static bool parse(const QString& text, DeviceAddress& out);
};

bool parseValueView(const QString& type, const DeviceAddress& addr, ValueView& out, QString& error);

#endif
