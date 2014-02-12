#ifndef DOMAINFACTORY_H
#define DOMAINFACTORY_H

namespace Ilwis {
class DomainHelper
{
public:
    DomainHelper();
    static Ilwis::IDomain create(Range* range);
    static IDomain fromTimeInterval(const QString &beg, const QString &end, const Duration& step=Duration(""));
    static IDomain fromNumericRange(double beg, double end, double step);
};
}

#endif // DOMAINFACTORY_H
