#include "QvCoreConfigOperations.h"

namespace Qv2ray
{
    namespace ConfigOperations
    {
        static const QStringList vLogLevels = {"none", "debug", "info", "warning", "error"};
        // -------------------------- BEGIN CONFIG GENERATIONS ----------------------------------------------------------------------------
        QJsonObject GenerateRoutes(bool enableProxy, bool cnProxy)
        {
            DROOT
            root.insert("domainStrategy", "IPIfNonMatch");
            //
            // For Rules list
            QJsonArray rulesList;

            if (!enableProxy) {
                // This is added to disable all proxies, as a alternative influence of #64
                rulesList.append(GenerateSingleRouteRule(QStringList() << "regexp:.*", true, OUTBOUND_TAG_DIRECT));
            }

            // Private IPs should always NOT TO PROXY!
            rulesList.append(GenerateSingleRouteRule(QStringList() << "geoip:private", false, OUTBOUND_TAG_DIRECT));
            //
            // Check if CN needs proxy, or direct.
            rulesList.append(GenerateSingleRouteRule(QStringList() << "geoip:cn", false, cnProxy ? OUTBOUND_TAG_PROXY : OUTBOUND_TAG_DIRECT));
            rulesList.append(GenerateSingleRouteRule(QStringList() << "geosite:cn", true, cnProxy ? OUTBOUND_TAG_PROXY :  OUTBOUND_TAG_DIRECT));
            //
            // As a bug fix of #64, this default rule has been disabled.
            //rulesList.append(GenerateSingleRouteRule(QStringList({"regexp:.*"}), true, globalProxy ? OUTBOUND_TAG_PROXY :  OUTBOUND_TAG_DIRECT));
            root.insert("rules", rulesList);
            RROOT
        }

        QJsonObject GenerateSingleRouteRule(QStringList list, bool isDomain, QString outboundTag, QString type)
        {
            DROOT
            root.insert(isDomain ? "domain" : "ip", QJsonArray::fromStringList(list));
            JADD(outboundTag, type)
            RROOT
        }

        QJsonObject GenerateFreedomOUT(QString domainStrategy, QString redirect, int userLevel)
        {
            DROOT
            JADD(domainStrategy, redirect, userLevel)
            RROOT
        }
        QJsonObject GenerateBlackHoleOUT(bool useHTTP)
        {
            DROOT
            QJsonObject resp;
            resp.insert("type", useHTTP ? "http" : "none");
            root.insert("response", resp);
            RROOT
        }

        QJsonObject GenerateShadowSocksOUT(QList<QJsonObject> servers)
        {
            DROOT
            QJsonArray x;

            foreach (auto server, servers) {
                x.append(server);
            }

            root.insert("servers", x);
            RROOT
        }

        QJsonObject GenerateShadowSocksServerOUT(QString email, QString address, int port, QString method, QString password, bool ota, int level)
        {
            DROOT
            JADD(email, address, port, method, password, level, ota)
            RROOT
        }

        QJsonObject GenerateDNS(bool withLocalhost, QStringList dnsServers)
        {
            DROOT
            QJsonArray servers(QJsonArray::fromStringList(dnsServers));

            if (withLocalhost) {
                // https://github.com/lhy0403/Qv2ray/issues/64
                // The fix patch didn't touch this line below.
                servers.append("localhost");
            }

            root.insert("servers", servers);
            RROOT
        }

        QJsonObject GenerateDokodemoIN(QString address, int port, QString  network, int timeout, bool followRedirect, int userLevel)
        {
            DROOT
            JADD(address, port, network, timeout, followRedirect, userLevel)
            RROOT
        }

        QJsonObject GenerateHTTPIN(QList<AccountObject> _accounts, int timeout, bool allowTransparent, int userLevel)
        {
            DROOT
            QJsonArray accounts;

            foreach (auto account, _accounts) {
                accounts.append(GetRootObject(account));
            }

            JADD(timeout, accounts, allowTransparent, userLevel)
            RROOT
        }

        QJsonObject GenerateSocksIN(QString auth, QList<AccountObject> _accounts, bool udp, QString ip, int userLevel)
        {
            DROOT
            QJsonArray accounts;

            foreach (auto acc, _accounts) {
                accounts.append(GetRootObject(acc));
            }

            JADD(auth, accounts, udp, ip, userLevel)
            RROOT
        }

        QJsonObject GenerateOutboundEntry(QString protocol, QJsonObject settings, QJsonObject streamSettings, QJsonObject mux, QString sendThrough, QString tag)
        {
            DROOT
            JADD(sendThrough, protocol, settings, tag, streamSettings, mux)
            RROOT
        }

        // -------------------------- END CONFIG GENERATIONS ------------------------------------------------------------------------------
        // BEGIN RUNTIME CONFIG GENERATION

        QJsonObject GenerateRuntimeConfig(QJsonObject root)
        {
            auto gConf = GetGlobalConfig();
            QJsonObject logObject;
            //
            //logObject.insert("access", QV2RAY_CONFIG_PATH + QV2RAY_VCORE_LOG_DIRNAME + QV2RAY_VCORE_ACCESS_LOG_FILENAME);
            //logObject.insert("error", QV2RAY_CONFIG_PATH + QV2RAY_VCORE_LOG_DIRNAME + QV2RAY_VCORE_ERROR_LOG_FILENAME);
            //
            logObject.insert("loglevel", vLogLevels[gConf.logLevel]);
            root.insert("log", logObject);
            //
            QStringList dnsList;

            foreach (auto str, gConf.dnsList) {
                dnsList.append(QString::fromStdString(str));
            }

            auto dnsObject = GenerateDNS(gConf.withLocalDNS, dnsList);
            root.insert("dns", dnsObject);
            //
            //
            root.insert("stats", QJsonObject());
            //
            QJsonArray inboundsList;

            // HTTP InBound
            if (gConf.inBoundSettings.http_port != 0) {
                QJsonObject httpInBoundObject;
                httpInBoundObject.insert("listen", QString::fromStdString(gConf.inBoundSettings.listenip));
                httpInBoundObject.insert("port", gConf.inBoundSettings.http_port);
                httpInBoundObject.insert("protocol", "http");
                httpInBoundObject.insert("tag", "http_IN");

                if (gConf.inBoundSettings.http_useAuth) {
                    auto httpInSettings =  GenerateHTTPIN(QList<AccountObject>() << gConf.inBoundSettings.httpAccount);
                    httpInBoundObject.insert("settings", httpInSettings);
                }

                inboundsList.append(httpInBoundObject);
            }

            // SOCKS InBound
            if (gConf.inBoundSettings.socks_port != 0) {
                QJsonObject socksInBoundObject;
                socksInBoundObject.insert("listen", QString::fromStdString(gConf.inBoundSettings.listenip));
                socksInBoundObject.insert("port", gConf.inBoundSettings.socks_port);
                socksInBoundObject.insert("protocol", "socks");
                socksInBoundObject.insert("tag", "socks_IN");
                auto socksInSettings = GenerateSocksIN(gConf.inBoundSettings.socks_useAuth ? "password" : "noauth", QList<AccountObject>() << gConf.inBoundSettings.socksAccount);
                socksInBoundObject.insert("settings", socksInSettings);
                inboundsList.append(socksInBoundObject);
            }

            if (!root.contains("inbounds") || root["inbounds"].toArray().empty()) {
                root.insert("inbounds", inboundsList);
            }

            // Note: The part below always makes the whole functionality in trouble......
            // BE EXTREME CAREFUL when changing these code below...

            // For SOME configs, there is no "route" entries, so, we add some...

            if (!root.contains("routing")) {
                if (root["outbounds"].toArray().count() != 1) {
                    // There are no ROUTING but 2 or more outbounds.... This is rare, but possible.
                    LOG(MODULE_CONNECTION, "WARN: This message usually indicates the config file has some logic errors:")
                    LOG(MODULE_CONNECTION, "WARN: --> The config file has NO routing section, however more than 1 outbounds are detected.")
                }

                LOG(MODULE_CONNECTION, "Current connection has NO ROUTING section, we insert default values.")
                auto routeObject = GenerateRoutes(gConf.enableProxy, gConf.proxyCN);
                root.insert("routing", routeObject);
                QJsonArray outbounds = root["outbounds"].toArray();
                outbounds.append(GenerateOutboundEntry("freedom", GenerateFreedomOUT("AsIs", ":0", 0), QJsonObject(), QJsonObject(), "0.0.0.0", OUTBOUND_TAG_DIRECT));
                // TODO
                //
                // We don't want to add MUX into the first one in the list.....
                // However, this can be added to the Connection Edit Window...
                //QJsonObject first = outbounds.first().toObject();
                //first.insert("mux", GetRootObject(gConf.mux));
                //outbounds[0] = first;
                //
                root["outbounds"] = outbounds;
            } else {
                // For some config files that has routing entries already.
                // We don't add extra routings.
                // this part has been left blanking
                LOG(MODULE_CONNECTION, "Skip adding 'freedom' entry.")
            }

            return root;
        }
    }
}
