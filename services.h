#ifndef SERVICES_H
#define SERVICES_H

#include <map>
#include <string>

struct ServiceInfo
{
    std::string name;
    bool passive;
    std::string probe;
    std::string bannerPattern;
};

std::map<int, ServiceInfo> serviceMap;

void initializeServices()
{
    serviceMap[21] = {"FTP", true, "", "^220"};
    serviceMap[22] = {"SSH", true, "", "SSH-"};
    serviceMap[23] = {"Telnet", true, "", ""};
    serviceMap[25] = {"SMTP", true, "", "^220"};
    serviceMap[53] = {"DNS", false, "", ""};
    serviceMap[80] = {"HTTP", false, "GET / HTTP/1.0\r\n\r\n", "HTTP/1\\."};
    serviceMap[110] = {"POP3", true, "", "^\\+OK"};
    serviceMap[143] = {"IMAP", true, "", "IMAP"};
    serviceMap[389] = {"LDAP", false, "", ""};
    serviceMap[443] = {"HTTPS", false, "", ""};
    serviceMap[445] = {"SMB", false, "", ""};
    serviceMap[3306] = {"MySQL", true, "", "MySQL"};
    serviceMap[3389] = {"RDP", false, "", ""};
    serviceMap[5432] = {"PostgreSQL", true, "", ""};
    serviceMap[5984] = {"CouchDB", false, "GET / HTTP/1.0\r\n\r\n", "CouchDB"};
    serviceMap[6379] = {"Redis", true, "", "REDIS"};
    serviceMap[8000] = {"HTTP-Alt", false, "GET / HTTP/1.0\r\n\r\n", "HTTP/1\\."};
    serviceMap[8080] = {"HTTP-Alt", false, "GET / HTTP/1.0\r\n\r\n", "HTTP/1\\."};
    serviceMap[8443] = {"HTTPS-Alt", false, "", ""};
    serviceMap[9200] = {"Elasticsearch", false, "GET / HTTP/1.0\r\n\r\n", "elasticsearch"};
    serviceMap[27017] = {"MongoDB", true, "", ""};
    serviceMap[135] = {"RPC", false, "", ""};
    serviceMap[139] = {"NetBIOS", false, "", ""};
    serviceMap[161] = {"SNMP", false, "", ""};
    serviceMap[162] = {"SNMP Trap", false, "", ""};
    serviceMap[179] = {"BGP", false, "", ""};
    serviceMap[514] = {"Syslog", true, "", ""};
    serviceMap[587] = {"SMTP-TLS", true, "", "^220"};
    serviceMap[993] = {"IMAP-SSL", false, "", ""};
    serviceMap[995] = {"POP3-SSL", false, "", ""};
    serviceMap[1433] = {"MSSQL", false, "", ""};
    serviceMap[1521] = {"Oracle", false, "", ""};
    serviceMap[2049] = {"NFS", false, "", ""};
    serviceMap[3000] = {"Node.js", false, "GET / HTTP/1.0\r\n\r\n", "HTTP/1\\."};
    serviceMap[5005] = {"JDWP", false, "", ""};
    serviceMap[5900] = {"VNC", true, "", "RFB"};
    serviceMap[9000] = {"SonarQube", false, "GET / HTTP/1.0\r\n\r\n", "SonarQube"};
    serviceMap[9090] = {"Prometheus", false, "GET / HTTP/1.0\r\n\r\n", "text/html"};
    serviceMap[10000] = {"Webmin", false, "GET / HTTP/1.0\r\n\r\n", "HTTP/1\\."};
    serviceMap[11211] = {"Memcached", true, "", ""};
    serviceMap[27015] = {"Source Engine", false, "", ""};
    serviceMap[28017] = {"MongoDB Web", false, "GET / HTTP/1.0\r\n\r\n", "HTTP/1\\."};
    serviceMap[32768] = {"Filestash", false, "GET / HTTP/1.0\r\n\r\n", "HTTP/1\\."};
}

#endif
