#pragma once

#include <string>
#include <utility>
#include <vector>

// Header blocks the allocation/footprint tests (header_alloc_tests.cpp) parse.
//
// The two responses are real captures from a browser session through the proxy
// (a very long Content-Security-Policy, and an HTTP/2-style lowercase block with
// many cookies and CORS lists). Everything that identifies a session -- login,
// uids, request ids, SID/DID, GUIDs -- was replaced by zeros/dummies of exactly
// the same length, so the byte counts stay what they were on the wire.
// The request is representative of a desktop browser, not a capture.
namespace HeaderSamples
{
  enum class Kind
  {
    Plain,
    Response,
    Request
  };

  struct Sample
  {
    std::string Name;
    std::string Text; // the block exactly as it is received, ends with the empty line
    bool LowerCase;   // how the Headers object that parses it is created
    Kind Type;

    Sample(std::string name, std::string text, bool lowerCase, Kind type)
      : Name(std::move(name))
      , Text(std::move(text))
      , LowerCase(lowerCase)
      , Type(type)
    {
    }
  };

  inline std::string Join(const std::vector<std::string>& lines)
  {
    std::string text;
    for (const auto& line : lines)
    {
      text += line;
      text += "\r\n";
    }

    text += "\r\n";
    return text;
  }

  inline std::string Dzen()
  {
    return Join({
      "HTTP/1.1 200 OK"
      , "Cache-Control: no-cache, no-store, max-age=0, must-revalidate"
      , "Content-Encoding: zstd"
      , std::string("Content-Security-Policy: ")
        + "child-src blob: mc.yandex.ru; "
        + "connect-src 'self' blob: *.admon.pro *.telecid.ru *.adjust.com *.adriver.ru *.buzzoola.com *.adplay.ru *.adlooxtracking.com *.adlooxtracking.ru *.adsafeprotected.com *.apptracer.ru *.cdn.dzeninfra.ru *.cdn.ngenix.net *.cold-video.dzeninfra.ru *.criteo.com *.criteo.net *.cu.dzen.ru *.doubleverify.com *.dzen.ru *.extcdn.dzeninfra.ru *.gstatic.com *.hot-video.dzeninfra.ru *.imgsmail.ru *.mail.ru *.moatads.com *.mradx.net *.ms.dzen.ru *.mycdn.me *.odkl.ru *.okcdn.ru *.s3.dzeninfra.ru *.serving-sys.com *.serving-sys.ru *.strm.yandex.net *.strm.yandex.ru *.tun.si.dzen.ru *.tunneler-si.dzen.ru *.verify.yandex.ru *.vkuser.net *.yandex.com *.yandex.net *.yandex.ru *.zeta.dzen.ru stats.idzn.ru *.adriver.ru *.sape.ru *.betweendigital.com adfox.yandex.ru ads.adfox.ru ads.bumlam.com ads.vk.com ads.vk.ru api-ext-vh.dzeninfra.ru api.stat.yandex-team.ru auto.ru ads6.adfox.ru amc.yandex.ru an.yandex.ru api.passport-test.yandex.ru api.passport.yandex.ru avatars.dzeninfra.ru avatars.mds.yandex.net awaps.yandex.net awaps.yandex.ru cdn-probe-jobs.mrgcdn.ru cdn-probe-reports.mrgcdn.ru cdn.dzen.ru cdn.dzeninfra.ru clck.dzen.ru *.a.mts.ru x01.aidata.io *.ad-score.com cloud-api.yandex.ru cold-video.dzeninfra.ru dzen.ru https://frontend.vh.yandex.org log.bumlam.com log.dzen.ru favicon.yandex.net fcm.googleapis.com fcmregistrations.googleapis.com firebaseinstallations.googleapis.com forms-ext-api.yandex.ru frontend.vh.yandex.ru http-check-headers.yandex.ru https://vh.test.yandex.ru/live/ jstracer.yandex.ru log.dzen.ru matchid.adfox.yandex.ru mc.admetrica.ru mc.webvisor.com mc.webvisor.org mc.yandex.az mc.yandex.by mc.yandex.co.il mc.yandex.com mc.yandex.com.am mc.yandex.com.ge mc.yandex.com.tr mc.yandex.ee mc.yandex.fr mc.yandex.kg mc.yandex.kz mc.yandex.lt mc.yandex.lv mc.yandex.md mc.yandex.ru mc.yandex.tj mc.yandex.tm mc.yandex.uz wss://mc.yandex.ru wss://mc.yandex.az wss://mc.yandex.by wss://mc.yandex.co.il wss://mc.yandex.com wss://mc.yandex.com.am wss://mc.yandex.com.ge wss://mc.yandex.com.tr wss://mc.yandex.ee wss://mc.yandex.fr wss://mc.yandex.kg wss://mc.yandex.kz wss://mc.yandex.lt wss://mc.yandex.lv wss://mc.yandex.md wss://mc.yandex.tj wss://mc.yandex.tm wss://mc.yandex.uz wss://mc.webvisor.com wss://mc.webvisor.org notify.dzen.ru pixel.adsafeprotected.com playlog.dzen.ru s3.dzeninfra.ru static-mon.yandex.net static.dzeninfra.ru zen-int.dzeninfra.ru strm.yandex.net strm.yandex.ru suggest-maps.yandex.ru/suggest-geo telemetry.dzen.ru tps.doubleverify.com verify.yandex.ru video.dzen.ru vk.ru wss://push.yandex.ru wss://mc.yandex.ru yandex.ru yandex.st yandex.net yandexmetrica.com yandexmetrica.com:29009 yandexmetrica.com:29010 yandexmetrica.com:30102 yandexmetrica.com:30103 yastat.net yastatic.net zen.me ymetrica1.com ymvccb7b8b.ru zen-rc3.yandex.ru *.yandex-team.ru sportsdzen.ru *.p.adserv.ai *.adhigh.net https://openrouter.ai/api/v1/chat/completions whitei.net *.cdn-vk.ru *.cdn-vk.net *.mrgcdn.ru *.mrgcdn.net *.supercdn.ru *.vkcdn.ru *.imgsmail.ru *.okcdn.ru; "
        + "default-src 'self' blob: dzen.ru cdn.dzen.ru cdn.dzeninfra.ru log.dzen.ru playlog.dzen.ru avatars.dzeninfra.ru cold-video.dzeninfra.ru *.cdn.dzeninfra.ru *.cold-video.dzeninfra.ru s3.dzeninfra.ru static.dzeninfra.ru zen-int.dzeninfra.ru *.admon.pro *.telecid.ru *.adjust.com *.adriver.ru *.betweendigital.com *.adplay.ru *.adlooxtracking.com *.adlooxtracking.ru *.adsafeprotected.com *.apptracer.ru *.buzzoola.com *.sape.ru *.criteo.com *.criteo.net *.cu.dzen.ru *.doubleverify.com *.extcdn.dzeninfra.ru *.hot-video.dzeninfra.ru *.imgsmail.ru *.mail.ru *.moatads.com *.mradx.net *.ms.dzen.ru *.s3.dzeninfra.ru *.serving-sys.com *.serving-sys.ru *.yandex.com *.yandex.net *.yandex.ru video.dzen.ru yandex.ru yastatic.net; "
        + "font-src 'self' data: *.admon.pro *.telecid.ru *.adjust.com *.adriver.ru *.buzzoola.com *.sape.ru *.betweendigital.com *.adplay.ru *.cu.dzen.ru *.yandex.com *.yandex.net *.yandex.ru static.dzeninfra.ru zen-int.dzeninfra.ru yastatic.net *.criteo.com *.criteo.net *.imgsmail.ru *.mail.ru *.mradx.net *.mycdn.me *.vkuser.net an.yandex.ru fonts.googleapis.com fonts.gstatic.com http-check-headers.yandex.ru www.gstatic.com yastat.net *.adhigh.net; "
        + "frame-ancestors 'self' *.cu.dzen.ru backwoods.yandex-team.ru dzen.ru *.dzen.ru iframe-toloka.com metrika.yandex.ru webvisor.com sq2.go.mail.ru toloka.yandex.com toloka.yandex.ru yang.yandex-team.ru *.webvisor.com http://*.webvisor.com http://webvisor.com; "
        + "frame-src 'self' blob: *.admon.pro *.telecid.ru *.adjust.com *.adriver.ru *.buzzoola.com *.sape.ru *.betweendigital.com *.adplay.ru *.criteo.com *.criteo.net *.doubleclick.net *.doubleverify.com *.dzen.ru *.imgsmail.ru *.mail.ru *.mradx.net *.yandex.com *.yandex.net *.mycdn.me *.tun.si.dzen.ru *.tunneler-si.dzen.ru *.vkuser.net *.yandex.ru *.yandexadexchange.net *.yastatic.net *.youtube.com *.adriver.ru ads.vk.com ads.vk.ru auto.ru rutube.ru *.ad-score.com awaps.yandex.net banners.adfox.ru *.vkpay.io *.vkpay.ru dzen.ru https://frontend.vh.yandex.org https://promo.avto.ru https://www.kinopoisk.ru https://www.tinkoff.ru http-check-headers.yandex.ru id.vk.com id.vk.ru login.vk.com login.vk.ru mc.yandex.com mc.yandex.md promo-money.ru imasdk.googleapis.com mc.yandex.ru sso.dzen.ru sso.passport.yandex.ru static.dzeninfra.ru zen-int.dzeninfra.ru vk.com vk.ru storage.mds.yandex.net suggest.dzen.ru vh-playerweb.s3.dzeninfra.ru yandex.ru yandexadexchange.net yastat.net yastatic.net yoomoney.ru youtu.be youtube.com zenadservices.net zenkit://* *.vk.com *.vk.ru m.vk.com m.vk.ru vkvideo.ru www.youtube.com yandex.com yandex.net yandex.st *.vk.team *.cdp-data.ru rutarget.ru *.adhigh.net ya.ru; "
        + "img-src 'self' * blob: data:; "
        + "manifest-src 'self' *.dzen.ru *.yandex.net *.yandex.ru dzen.ru sportsdzen.ru static.dzeninfra.ru zen-int.dzeninfra.ru yandex.net yastatic.net; "
        + "media-src 'self' blob: data: *.admon.pro *.telecid.ru *.adjust.com *.adriver.ru *.buzzoola.com *.sape.ru *.betweendigital.com *.adplay.ru *.cdn.dzeninfra.ru *.cdn.ngenix.net *.cold-video.dzeninfra.ru *.criteo.com *.criteo.net *.cu.dzen.ru *.extcdn.dzeninfra.ru *.hot-video.dzeninfra.ru *.imgsmail.ru *.mail.ru *.mradx.net *.ms.dzen.ru *.mycdn.me *.okcdn.ru *.s3.dzeninfra.ru *.strm.yandex.net *.strm.yandex.ru *.targetads.io *.yandex.com *.tun.si.dzen.ru *.tunneler-si.dzen.ru *.vkuser.net *.yandex.net *.yandex.ru *.zeta.dzen.ru banners.adfox.ru cdn.dzen.ru cdn3.terratraf.io cold-video.dzeninfra.ru content.adfox.ru dzen.ru http-check-headers.yandex.ru r.mradx.net s3.dzeninfra.ru static.dzeninfra.ru zen-int.dzeninfra.ru strm.yandex.ru video.dzen.ru yandex.net yandex.ru yandex.st yastat.net yastatic.net; "
        + "object-src 'none'; "
        + "script-src 'self' 'unsafe-eval' 'unsafe-inline' blob: *.ad-score.com *.adlooxtracking.com *.adlooxtracking.ru *.adplay.ru *.admon.pro *.telecid.ru *.adjust.com *.adriver.ru *.ads.betweendigital.com *.adsafeprotected.com *.betweendigital.com *.buzzoola.com *.criteo.com *.criteo.net *.cu.dzen.ru *.doubleclick.net *.doubleverify.com *.dvtps.com *.gstatic.com *.hit.gemius.pl *.imgsmail.ru *.mail.ru *.moatads.com *.mradx.net *.ms.dzen.ru *.mycdn.me *.s3.mds.yandex.net *.sape.ru *.serving-sys.ru *.tun.si.dzen.ru *.tunneler-si.dzen.ru *.userapi.com *.vk.com *.vk.ru *.vkuser.net *.yandex.com *.yandex.net *.yandex.ru *.zen.yandex.com ad.mail.ru ads.adfox.ru ads6.adfox.ru an.yandex.ru banners.adfox.ru *.sape.ru chat.s3.yandex.net dzen.ru http-check-headers.yandex.ru imasdk.googleapis.com mc.webvisor.com mc.webvisor.org mc.yandex.az mc.yandex.by mc.yandex.co.il mc.yandex.com mc.yandex.com.am mc.yandex.com.ge mc.yandex.com.tr mc.yandex.ee mc.yandex.fr mc.yandex.kg mc.yandex.kz mc.yandex.lt mc.yandex.lv mc.yandex.md mc.yandex.ru mc.yandex.tj mc.yandex.tm mc.yandex.ua mc.yandex.uz sso.dzen.ru sso.passport.yandex.ru static.dzeninfra.ru storage.mds.yandex.net top-fwz1.mail.ru vk.com vk.ru www.gstatic.com/cast/sdk/libs/ www.gstatic.com/cv/js/sender/ www.gstatic.com/eureka/clank/ www.tns-counter.ru yandex.com yandex.net yandex.ru yandex.st yastat.net yastatic.net z.moatads.com zen-int.dzeninfra.ru 'nonce-DBfeTAJZ7tFz1AjncwOznAJdAj9Wiaie' 'strict-dynamic'; "
        + "style-src 'self' 'unsafe-eval' 'unsafe-inline' *.admon.pro *.telecid.ru *.adjust.com *.adriver.ru *.buzzoola.com *.sape.ru *.betweendigital.com *.adplay.ru *.criteo.com *.criteo.net *.cu.dzen.ru *.imgsmail.ru *.mail.ru *.mradx.net *.ms.dzen.ru *.yandex.com *.yandex.net *.yandex.ru *.mycdn.me *.tun.si.dzen.ru *.tunneler-si.dzen.ru *.vkuser.net *.zen.yandex.com banners.adfox.ru content.adfox.ru dzen.ru http-check-headers.yandex.ru static.dzeninfra.ru zen-int.dzeninfra.ru yandex.com yandex.net yandex.ru yandex.st yastat.net yastatic.net fonts.googleapis.com; "
        + "worker-src 'self' *.dzen.ru blob: dzen.ru *.apptracer.ru; "
        + "report-uri https://csp.yandex.net/csp?from=zen_old&project=zen&yandex_login=user00&yandexuid=1000000000000000000&requestid=00000000000000000000000000000000&page=site_desktop;"
      , "Content-Type: text/html; charset=utf-8"
      , "Date: Sun, 04 Oct 2026 04:02:05 GMT"
      , "Transfer-Encoding: chunked"
      , "Vary: Accept-Encoding"
      , "X-Content-Type-Options: nosniff"
      , "X-Request-Id: 00000000000000000000000000000000"
      , "X-Requestid: 0000000000.0000.0000000000000.00000"
      , "X-Xss-Protection: 1; mode=block"
    });
  }

  // HTTP/2-style block as the proxy logs it: pseudo header first, all names
  // lowercase (so parsed by a lower-casing Headers object).
  inline std::string Msn()
  {
    const std::string list =
      "TicketType,RequestContinuationKey,AuthToken,Content-Type,x-client-activityid,ms-cv,OneSvc-Uni-Feat-Tun,signedInCookieName,muid,appid,User-Location,user-location,userauthtoken,usertickettype,sitename,s2sauthtoken,thumbprint,Authorization,Ent-Authorization,UserIdToken,DDD-TMPL,DDD-ActivityId,DDD-FeatureSet,DDD-Session-ID,Date,date,ads-referer,ads-referer,taboola-sessionId,taboola-sessionid,Akamai-Request-ID,Akamai-Server-IP,X-MSEdge-Ref,DDD-DebugId,s-xbox-token,OneWebServiceLatency,X-FD-Features,DDD-UserType,traceparent,Widgets,Muted,Velocity,DDD-Auth-Features,SoftLanding,PrefMigrated,DDD-TMPL-Removed,deviceFeatures,Server-Timing,X-1S-Digital-ID,x-ntp-cohort-prefs,x-kunlun-trace-id,x-kunlun-session-id,x-kunlun-request-id,x-kunlun-client-id";

    return Join({
      ":status: 200"
      , "content-type: application/json; charset=utf-8"
      , "content-encoding: gzip"
      , "vary: Accept-Encoding"
      , "set-cookie: _C_ETH=1; domain=.msn.com; path=/; secure; httponly"
      , "set-cookie: _C_Auth="
      , "set-cookie: _EDGE_S=SID=00000000000000000000000000000000; domain=.msn.com; path=/; httponly"
      , "access-control-allow-credentials: true"
      , "access-control-allow-headers: " + list
      , "access-control-allow-methods: PUT,PATCH,POST,GET,OPTIONS,DELETE"
      , "access-control-allow-origin: *.msn.com"
      , "access-control-expose-headers: " + list
      , "ddd-authenticatedwithjwtflow: False"
      , "ddd-usertype: AnonymousMuid"
      , "ddd-strategyexecutionlatency: 00:00:00.4459538"
      , "x-wpo-activityid: 00000000-0000-0000-0000-000000000000|2026-09-26T02:40:48.6695472Z|fabric_wpo|FRC-C|WPO_611"
      , "ddd-activityid: 00000000-0000-0000-0000-000000000000"
      , "ddd-feednewsitemcount: 0"
      , "ddd-tmpl: P1DynaDura;BingRecoCode:Success;IsRecoNewUser:1;P1DeviceWithEL;msnup_cnex:no;TileID:txwt;sportsTbr:Sports_SportsMatch_LiveGame_00000000-0000-0000-0000-000000000000_00000000-0000-0000-0000-000000000000|00000000-0000-0000-0000-000000000000|00000000-0000-0000-0000-000000000000|EntityId~00000000-0000-0000-0000-000000000000|explorationReason~2|contentId~00000000-0000-0000-0000-000000000000~00000000-0000-0000-0000-000000000000~InProgress~6-2|exp~1||rel_0||wrt_2.62;XFeed;partialResponse:1;RR:0;wxcdid:25;PageViewCount0;sportsWidget:SportsMatch_Cricket_00000000-0000-0000-0000-000000000000_0000000000000000||rel_0||wrt_2.3;WxCardValid:1;wxpkg:2.459.0;wxunt:_C;WxLockScreen:Weather3DLock"
      , "ddd-tmpl-removed: False"
      , "ddd-debugid: 00000000-0000-0000-0000-000000000000|2026-09-26T02:40:48.6965786Z|fabric_winfeed|FRC-C|WinFeed_1731"
      , "ddd-auth-features: AT:NA;DID:m-00000000000000000000000000000000;IT:Unknown;MuidStateOrigin:MuidFromHeader"
      , "onewebservicelatency: 447"
      , "x-msedge-responseinfo: 447"
      , "x-ceto-ref: 00000000000000000000000000000000|AFD:00000000000000000000000000000000|2026-09-26T02:40:48.245Z"
      , R"(nel: {"report_to":"network-errors","max_age":604800,"success_fraction":0.001,"failure_fraction":1.0})"
      , R"(report-to: {"group":"network-errors","max_age":604800,"endpoints":[{"url":"https://deff.nelreports.net/api/report?cat=msn"}]})"
      , "x-cache: CONFIG_NOCACHE"
      , "accept-ch: Sec-CH-UA-Arch, Sec-CH-UA-Bitness, Sec-CH-UA-Full-Version, Sec-CH-UA-Full-Version-List, Sec-CH-UA-Mobile, Sec-CH-UA-Model, Sec-CH-UA-Platform, Sec-CH-UA-Platform-Version"
      , "x-msedge-ref: Ref A: 00000000000000000000000000000000 Ref B: FRA261110505054 Ref C: 2026-09-26T02:40:48Z"
      , "date: Sat, 26 Sep 2026 02:40:47 GMT"
      , "nfs-req-id: 00000000-0000-0000-0000-000000000000"
      , "nfs-req-idx: 0"
      , "nfs-req-rid: 1"
      , "nfs-req-parent: 00000000-0000-0000-0000-000000000000"
    });
  }

  inline std::string BrowserRequest()
  {
    return Join({
      "GET /search?q=headers&hl=ru HTTP/1.1"
      , "Host: www.example.org"
      , "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/129.0.0.0 Safari/537.36"
      , "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7"
      , "Accept-Language: ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7"
      , "Accept-Encoding: gzip, deflate, br, zstd"
      , "Referer: https://www.example.org/"
      , "Cookie: session=0000000000000000000000000000000000000000; theme=dark; consent=1; _ga=GA1.2.0000000000.0000000000; _gid=GA1.2.0000000000.0000000000"
      , "Connection: keep-alive"
      , "Upgrade-Insecure-Requests: 1"
      , "Sec-Fetch-Dest: document"
      , "Sec-Fetch-Mode: navigate"
      , "Sec-Fetch-Site: same-origin"
      , "Sec-Fetch-User: ?1"
      , R"(Sec-Ch-Ua: "Chromium";v="129", "Not=A?Brand";v="8")"
      , "Sec-Ch-Ua-Mobile: ?0"
      , R"(Sec-Ch-Ua-Platform: "Windows")"
      , "Priority: u=0, i"
    });
  }

  inline std::string Tiny()
  {
    return Join({
      "HTTP/1.1 204 No Content"
      , "Server: nginx"
      , "Date: Sat, 04 Oct 2026 04:02:05 GMT"
    });
  }

  // As many distinct names as the 48K header block limit leaves room for.
  inline std::string Hostile()
  {
    std::vector<std::string> lines;
    lines.push_back("HTTP/1.1 200 OK");

    for (int i = 0; i < 1500; ++i)
    {
      lines.push_back("X-Header-" + std::to_string(i) + ": value-" + std::to_string(i));
    }

    return Join(lines);
  }

  inline std::vector<Sample> All()
  {
    std::vector<Sample> samples;
    samples.emplace_back("tiny response", Tiny(), false, Kind::Response);
    samples.emplace_back("dzen.ru response (huge CSP)", Dzen(), false, Kind::Response);
    samples.emplace_back("msn.com h2-style (lowercase)", Msn(), true, Kind::Plain);
    samples.emplace_back("browser request", BrowserRequest(), false, Kind::Request);
    samples.emplace_back("1500 distinct names", Hostile(), false, Kind::Response);
    return samples;
  }
}
