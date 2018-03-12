// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/test/histogram_tester.h"
#include "chrome/browser/page_load_metrics/metrics_web_contents_observer.h"
#include "chrome/browser/page_load_metrics/observers/core_page_load_metrics_observer.h"
#include "chrome/browser/page_load_metrics/observers/document_write_page_load_metrics_observer.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

class MetricsWebContentsObserverBrowserTest : public InProcessBrowserTest {
 public:
  MetricsWebContentsObserverBrowserTest() {}
  ~MetricsWebContentsObserverBrowserTest() override {}

 protected:
  base::HistogramTester histogram_tester_;

  DISALLOW_COPY_AND_ASSIGN(MetricsWebContentsObserverBrowserTest);
};

IN_PROC_BROWSER_TEST_F(MetricsWebContentsObserverBrowserTest, NoNavigation) {
  ASSERT_TRUE(embedded_test_server()->Start());

  histogram_tester_.ExpectTotalCount(internal::kHistogramCommit, 0);
  histogram_tester_.ExpectTotalCount(internal::kHistogramDomContentLoaded, 0);
  histogram_tester_.ExpectTotalCount(internal::kHistogramLoad, 0);
  histogram_tester_.ExpectTotalCount(internal::kHistogramFirstLayout, 0);
}

IN_PROC_BROWSER_TEST_F(MetricsWebContentsObserverBrowserTest, NewPage) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title1.html"));
  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title2.html"));

  histogram_tester_.ExpectTotalCount(internal::kHistogramDomContentLoaded, 1);
  histogram_tester_.ExpectTotalCount(internal::kHistogramLoad, 1);
  histogram_tester_.ExpectTotalCount(internal::kHistogramFirstLayout, 1);
  histogram_tester_.ExpectTotalCount(internal::kHistogramParseDuration, 1);
  histogram_tester_.ExpectTotalCount(
      internal::kHistogramParseBlockedOnScriptLoad, 1);
}

IN_PROC_BROWSER_TEST_F(MetricsWebContentsObserverBrowserTest,
                       SamePageNavigation) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title1.html"));
  ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("/title1.html#hash"));
  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title2.html"));

  histogram_tester_.ExpectTotalCount(internal::kHistogramDomContentLoaded, 1);
  histogram_tester_.ExpectTotalCount(internal::kHistogramLoad, 1);
  histogram_tester_.ExpectTotalCount(internal::kHistogramFirstLayout, 1);
}

IN_PROC_BROWSER_TEST_F(MetricsWebContentsObserverBrowserTest,
                       SameUrlNavigation) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title1.html"));
  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title1.html"));
  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title2.html"));

  // We expect one histogram sample for each navigation to title1.html.
  histogram_tester_.ExpectTotalCount(internal::kHistogramCommit, 2);
  histogram_tester_.ExpectTotalCount(internal::kHistogramDomContentLoaded, 2);
  histogram_tester_.ExpectTotalCount(internal::kHistogramLoad, 2);
  histogram_tester_.ExpectTotalCount(internal::kHistogramFirstLayout, 2);
}

IN_PROC_BROWSER_TEST_F(MetricsWebContentsObserverBrowserTest,
                       NonHtmlMainResource) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/simple.svg"));
  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title1.html"));

  histogram_tester_.ExpectTotalCount(internal::kHistogramCommit, 0);
}

IN_PROC_BROWSER_TEST_F(MetricsWebContentsObserverBrowserTest,
                       NonHttpOrHttpsUrl) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ui_test_utils::NavigateToURL(browser(), GURL(chrome::kChromeUIVersionURL));
  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title1.html"));

  histogram_tester_.ExpectTotalCount(internal::kHistogramCommit, 0);
}

IN_PROC_BROWSER_TEST_F(MetricsWebContentsObserverBrowserTest,
                       PreloadDocumentWrite) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL(
                     "/page_load_metrics/document_write_external_script.html"));
  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title2.html"));

  histogram_tester_.ExpectTotalCount(
      internal::kHistogramDocWriteParseStartToFirstContentfulPaint, 1);
}

IN_PROC_BROWSER_TEST_F(MetricsWebContentsObserverBrowserTest,
                       NoPreloadDocumentWrite) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL(
                     "/page_load_metrics/document_write_no_script.html"));
  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title2.html"));

  histogram_tester_.ExpectTotalCount(
      internal::kHistogramDocWriteParseStartToFirstContentfulPaint, 0);
}

IN_PROC_BROWSER_TEST_F(MetricsWebContentsObserverBrowserTest, NoDocumentWrite) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title1.html"));
  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title2.html"));
  histogram_tester_.ExpectTotalCount(
      internal::kHistogramDocWriteParseStartToFirstContentfulPaint, 0);
  histogram_tester_.ExpectTotalCount(
      internal::kHistogramDocWriteBlockParseStartToFirstContentfulPaint, 0);
}

IN_PROC_BROWSER_TEST_F(MetricsWebContentsObserverBrowserTest,
                       DocumentWriteBlock) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL(
                     "/page_load_metrics/document_write_script_block.html"));

  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title2.html"));

  histogram_tester_.ExpectTotalCount(
      internal::kHistogramDocWriteBlockParseStartToFirstContentfulPaint, 1);
}

IN_PROC_BROWSER_TEST_F(MetricsWebContentsObserverBrowserTest,
                       DocumentWriteReload) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL(
                     "/page_load_metrics/document_write_script_block.html"));

  // Reload should not log the histogram as the script is not blocked.
  ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL(
                     "/page_load_metrics/document_write_script_block.html"));

  ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL(
                     "/page_load_metrics/document_write_script_block.html"));

  histogram_tester_.ExpectTotalCount(
      internal::kHistogramDocWriteBlockParseStartToFirstContentfulPaint, 1);

  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title2.html"));

  histogram_tester_.ExpectTotalCount(
      internal::kHistogramDocWriteBlockParseStartToFirstContentfulPaint, 1);

  histogram_tester_.ExpectTotalCount(
      internal::kHistogramDocWriteBlockReloadCount, 2);
}

IN_PROC_BROWSER_TEST_F(MetricsWebContentsObserverBrowserTest,
                       DocumentWriteAsync) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL(
                     "/page_load_metrics/document_write_script_async.html"));
  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title2.html"));

  histogram_tester_.ExpectTotalCount(
      internal::kHistogramDocWriteBlockParseStartToFirstContentfulPaint, 0);
}

IN_PROC_BROWSER_TEST_F(MetricsWebContentsObserverBrowserTest,
                       DocumentWriteSameDomain) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL(
                     "/page_load_metrics/document_write_external_script.html"));
  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title2.html"));

  histogram_tester_.ExpectTotalCount(
      internal::kHistogramDocWriteBlockParseStartToFirstContentfulPaint, 0);
}

IN_PROC_BROWSER_TEST_F(MetricsWebContentsObserverBrowserTest,
                       NoDocumentWriteScript) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL(
                     "/page_load_metrics/document_write_no_script.html"));
  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title2.html"));

  histogram_tester_.ExpectTotalCount(
      internal::kHistogramDocWriteBlockParseStartToFirstContentfulPaint, 0);
}

IN_PROC_BROWSER_TEST_F(MetricsWebContentsObserverBrowserTest, BadXhtml) {
  ASSERT_TRUE(embedded_test_server()->Start());

  // When an XHTML page contains invalid XML, it causes a paint of the error
  // message without a layout. Page load metrics currently treats this as an
  // error. Eventually, we'll fix this by special casing the handling of
  // documents with non-well-formed XML on the blink side. See crbug.com/627607
  // for more.
  ui_test_utils::NavigateToURL(
      browser(),
      embedded_test_server()->GetURL("/page_load_metrics/badxml.xhtml"));
  NavigateToUntrackedUrl();

  histogram_tester_.ExpectTotalCount(internal::kHistogramFirstLayout, 0);
  histogram_tester_.ExpectTotalCount(internal::kHistogramFirstPaint, 0);
  histogram_tester_.ExpectBucketCount(page_load_metrics::internal::kErrorEvents,
                                      page_load_metrics::ERR_BAD_TIMING_IPC, 1);
}

IN_PROC_BROWSER_TEST_F(MetricsWebContentsObserverBrowserTest,
                       FirstMeaningfulPaintRecorded) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title1.html"));

  // Wait until the renderer finishes observing layouts.
  const int kNetworkIdleTime = 500;
  const int kMargin = 500;
  const std::string javascript = base::StringPrintf(
      "setTimeout(() => window.domAutomationController.send(true), %d)",
      kNetworkIdleTime + kMargin);
  bool result;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      browser()->tab_strip_model()->GetActiveWebContents(),
      javascript, &result));
  EXPECT_TRUE(result);

  NavigateToUntrackedUrl();
  histogram_tester_.ExpectUniqueSample(
      internal::kHistogramFirstMeaningfulPaintStatus,
      internal::FIRST_MEANINGFUL_PAINT_RECORDED, 1);
  histogram_tester_.ExpectTotalCount(
      internal::kHistogramFirstMeaningfulPaint, 1);
  histogram_tester_.ExpectTotalCount(
      internal::kHistogramParseStartToFirstMeaningfulPaint, 1);
}

IN_PROC_BROWSER_TEST_F(MetricsWebContentsObserverBrowserTest,
                       FirstMeaningfulPaintNotRecorded) {
  ASSERT_TRUE(embedded_test_server()->Start());

  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/title1.html"));

  // Navigate away before a FMP is reported.
  NavigateToUntrackedUrl();

  histogram_tester_.ExpectUniqueSample(
      internal::kHistogramFirstMeaningfulPaintStatus,
      internal::FIRST_MEANINGFUL_PAINT_DID_NOT_REACH_NETWORK_STABLE, 1);
  histogram_tester_.ExpectTotalCount(
      internal::kHistogramFirstMeaningfulPaint, 0);
  histogram_tester_.ExpectTotalCount(
      internal::kHistogramParseStartToFirstMeaningfulPaint, 0);
}
