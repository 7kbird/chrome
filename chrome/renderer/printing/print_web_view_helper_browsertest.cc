// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/run_loop.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/print_messages.h"
#include "chrome/renderer/printing/mock_printer.h"
#include "chrome/renderer/printing/print_web_view_helper.h"
#include "chrome/test/base/chrome_render_view_test.h"
#include "content/public/renderer/render_view.h"
#include "ipc/ipc_listener.h"
#include "printing/print_job_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebRange.h"
#include "third_party/WebKit/public/web/WebView.h"

#if defined(OS_WIN) || defined(OS_MACOSX)
#include "base/file_util.h"
#include "printing/image.h"

using blink::WebFrame;
using blink::WebLocalFrame;
using blink::WebString;
#endif

namespace printing {

namespace {

// A simple web page.
const char kHelloWorldHTML[] = "<body><p>Hello World!</p></body>";

#if !defined(OS_CHROMEOS)

// A simple webpage with a button to print itself with.
const char kPrintOnUserAction[] =
    "<body>"
    "  <button id=\"print\" onclick=\"window.print();\">Hello World!</button>"
    "</body>";

// HTML with 3 pages.
const char kMultipageHTML[] =
  "<html><head><style>"
  ".break { page-break-after: always; }"
  "</style></head>"
  "<body>"
  "<div class='break'>page1</div>"
  "<div class='break'>page2</div>"
  "<div>page3</div>"
  "</body></html>";

// A simple web page with print page size css.
const char kHTMLWithPageSizeCss[] =
    "<html><head><style>"
    "@media print {"
    "  @page {"
    "     size: 4in 4in;"
    "  }"
    "}"
    "</style></head>"
    "<body>Lorem Ipsum:"
    "</body></html>";

// A simple web page with print page layout css.
const char kHTMLWithLandscapePageCss[] =
    "<html><head><style>"
    "@media print {"
    "  @page {"
    "     size: landscape;"
    "  }"
    "}"
    "</style></head>"
    "<body>Lorem Ipsum:"
    "</body></html>";

// A longer web page.
const char kLongPageHTML[] =
    "<body><img src=\"\" width=10 height=10000 /></body>";

// A web page to simulate the print preview page.
const char kPrintPreviewHTML[] =
    "<body><p id=\"pdf-viewer\">Hello World!</p></body>";

void CreatePrintSettingsDictionary(base::DictionaryValue* dict) {
  dict->SetBoolean(kSettingLandscape, false);
  dict->SetBoolean(kSettingCollate, false);
  dict->SetInteger(kSettingColor, GRAY);
  dict->SetBoolean(kSettingPrintToPDF, true);
  dict->SetInteger(kSettingDuplexMode, SIMPLEX);
  dict->SetInteger(kSettingCopies, 1);
  dict->SetString(kSettingDeviceName, "dummy");
  dict->SetInteger(kPreviewUIID, 4);
  dict->SetInteger(kPreviewRequestID, 12345);
  dict->SetBoolean(kIsFirstRequest, true);
  dict->SetInteger(kSettingMarginsType, DEFAULT_MARGINS);
  dict->SetBoolean(kSettingPreviewModifiable, false);
  dict->SetBoolean(kSettingHeaderFooterEnabled, false);
  dict->SetBoolean(kSettingGenerateDraftData, true);
  dict->SetBoolean(kSettingShouldPrintBackgrounds, false);
  dict->SetBoolean(kSettingShouldPrintSelectionOnly, false);
}
#endif  // !defined(OS_CHROMEOS)

class DidPreviewPageListener : public IPC::Listener {
 public:
  explicit DidPreviewPageListener(base::RunLoop* run_loop)
      : run_loop_(run_loop) {}

  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE {
    if (message.type() == PrintHostMsg_MetafileReadyForPrinting::ID ||
        message.type() == PrintHostMsg_PrintPreviewFailed::ID ||
        message.type() == PrintHostMsg_PrintPreviewCancelled::ID)
      run_loop_->Quit();
    return false;
  }

 private:
  base::RunLoop* const run_loop_;
  DISALLOW_COPY_AND_ASSIGN(DidPreviewPageListener);
};

}  // namespace

class PrintWebViewHelperTestBase : public ChromeRenderViewTest {
 public:
  PrintWebViewHelperTestBase() {}
  virtual ~PrintWebViewHelperTestBase() {}

 protected:
  void PrintWithJavaScript() {
    ExecuteJavaScript("window.print();");
    ProcessPendingMessages();
  }
  // The renderer should be done calculating the number of rendered pages
  // according to the specified settings defined in the mock render thread.
  // Verify the page count is correct.
  void VerifyPageCount(int count) {
#if defined(OS_CHROMEOS)
    // The DidGetPrintedPagesCount message isn't sent on ChromeOS. Right now we
    // always print all pages, and there are checks to that effect built into
    // the print code.
#else
    const IPC::Message* page_cnt_msg =
        render_thread_->sink().GetUniqueMessageMatching(
            PrintHostMsg_DidGetPrintedPagesCount::ID);
    ASSERT_TRUE(page_cnt_msg);
    PrintHostMsg_DidGetPrintedPagesCount::Param post_page_count_param;
    PrintHostMsg_DidGetPrintedPagesCount::Read(page_cnt_msg,
                                               &post_page_count_param);
    EXPECT_EQ(count, post_page_count_param.b);
#endif  // defined(OS_CHROMEOS)
  }

  // The renderer should be done calculating the number of rendered pages
  // according to the specified settings defined in the mock render thread.
  // Verify the page count is correct.
  void VerifyPreviewPageCount(int count) {
    const IPC::Message* page_cnt_msg =
        render_thread_->sink().GetUniqueMessageMatching(
        PrintHostMsg_DidGetPreviewPageCount::ID);
    ASSERT_TRUE(page_cnt_msg);
    PrintHostMsg_DidGetPreviewPageCount::Param post_page_count_param;
    PrintHostMsg_DidGetPreviewPageCount::Read(page_cnt_msg,
                                              &post_page_count_param);
    EXPECT_EQ(count, post_page_count_param.a.page_count);
  }

  // Verifies whether the pages printed or not.
  void VerifyPagesPrinted(bool printed) {
#if defined(OS_CHROMEOS)
    bool did_print_msg = (render_thread_->sink().GetUniqueMessageMatching(
        PrintHostMsg_TempFileForPrintingWritten::ID) != NULL);
    ASSERT_EQ(printed, did_print_msg);
#else
    const IPC::Message* print_msg =
        render_thread_->sink().GetUniqueMessageMatching(
            PrintHostMsg_DidPrintPage::ID);
    bool did_print_msg = (NULL != print_msg);
    ASSERT_EQ(printed, did_print_msg);
    if (printed) {
      PrintHostMsg_DidPrintPage::Param post_did_print_page_param;
      PrintHostMsg_DidPrintPage::Read(print_msg, &post_did_print_page_param);
      EXPECT_EQ(0, post_did_print_page_param.a.page_number);
    }
#endif  // defined(OS_CHROMEOS)
  }
  void OnPrintPages() {
    PrintWebViewHelper::Get(view_)->OnPrintPages();
    ProcessPendingMessages();
  }

  void VerifyPreviewRequest(bool requested) {
    const IPC::Message* print_msg =
        render_thread_->sink().GetUniqueMessageMatching(
            PrintHostMsg_SetupScriptedPrintPreview::ID);
    bool did_print_msg = (NULL != print_msg);
    ASSERT_EQ(requested, did_print_msg);
  }

  void OnPrintPreview(const base::DictionaryValue& dict) {
    PrintWebViewHelper* print_web_view_helper = PrintWebViewHelper::Get(view_);
    print_web_view_helper->OnInitiatePrintPreview(false);
    base::RunLoop run_loop;
    DidPreviewPageListener filter(&run_loop);
    render_thread_->sink().AddFilter(&filter);
    print_web_view_helper->OnPrintPreview(dict);
    run_loop.Run();
    render_thread_->sink().RemoveFilter(&filter);
  }

  void OnPrintForPrintPreview(const base::DictionaryValue& dict) {
    PrintWebViewHelper::Get(view_)->OnPrintForPrintPreview(dict);
    ProcessPendingMessages();
  }

  DISALLOW_COPY_AND_ASSIGN(PrintWebViewHelperTestBase);
};

class PrintWebViewHelperTest : public PrintWebViewHelperTestBase {
 public:
  PrintWebViewHelperTest() {}
  virtual ~PrintWebViewHelperTest() {}

  virtual void SetUp() OVERRIDE {
    ChromeRenderViewTest::SetUp();
  }

 protected:
  DISALLOW_COPY_AND_ASSIGN(PrintWebViewHelperTest);
};

// Tests that printing pages work and sending and receiving messages through
// that channel all works.
TEST_F(PrintWebViewHelperTest, OnPrintPages) {
  LoadHTML(kHelloWorldHTML);
  OnPrintPages();

  VerifyPageCount(1);
  VerifyPagesPrinted(true);
}

#if (defined(OS_WIN) && !WIN_PDF_METAFILE_FOR_PRINTING) || defined(OS_MACOSX)
// TODO(estade): I don't think this test is worth porting to Linux. We will have
// to rip out and replace most of the IPC code if we ever plan to improve
// printing, and the comment below by sverrir suggests that it doesn't do much
// for us anyway.
TEST_F(PrintWebViewHelperTest, PrintWithIframe) {
  // Document that populates an iframe.
  const char html[] =
      "<html><body>Lorem Ipsum:"
      "<iframe name=\"sub1\" id=\"sub1\"></iframe><script>"
      "  document.write(frames['sub1'].name);"
      "  frames['sub1'].document.write("
      "      '<p>Cras tempus ante eu felis semper luctus!</p>');"
      "</script></body></html>";

  LoadHTML(html);

  // Find the frame and set it as the focused one.  This should mean that that
  // the printout should only contain the contents of that frame.
  WebFrame* sub1_frame =
      view_->GetWebView()->findFrameByName(WebString::fromUTF8("sub1"));
  ASSERT_TRUE(sub1_frame);
  view_->GetWebView()->setFocusedFrame(sub1_frame);
  ASSERT_NE(view_->GetWebView()->focusedFrame(),
            view_->GetWebView()->mainFrame());

  // Initiate printing.
  OnPrintPages();
  VerifyPagesPrinted(true);

  // Verify output through MockPrinter.
  const MockPrinter* printer(chrome_render_thread_->printer());
  ASSERT_EQ(1, printer->GetPrintedPages());
  const Image& image1(printer->GetPrintedPage(0)->image());

  // TODO(sverrir): Figure out a way to improve this test to actually print
  // only the content of the iframe.  Currently image1 will contain the full
  // page.
  EXPECT_NE(0, image1.size().width());
  EXPECT_NE(0, image1.size().height());
}
#endif

// Tests if we can print a page and verify its results.
// This test prints HTML pages into a pseudo printer and check their outputs,
// i.e. a simplified version of the PrintingLayoutTextTest UI test.
namespace {
// Test cases used in this test.
struct TestPageData {
  const char* page;
  size_t printed_pages;
  int width;
  int height;
  const char* checksum;
  const wchar_t* file;
};

#if defined(OS_WIN) || defined(OS_MACOSX)
const TestPageData kTestPages[] = {
  {"<html>"
  "<head>"
  "<meta"
  "  http-equiv=\"Content-Type\""
  "  content=\"text/html; charset=utf-8\"/>"
  "<title>Test 1</title>"
  "</head>"
  "<body style=\"background-color: white;\">"
  "<p style=\"font-family: arial;\">Hello World!</p>"
  "</body>",
#if defined(OS_MACOSX)
  // Mac printing code compensates for the WebKit scale factor while generating
  // the metafile, so we expect smaller pages.
  1, 600, 780,
#else
  1, 675, 900,
#endif
  NULL,
  NULL,
  },
};
#endif  // defined(OS_WIN) || defined(OS_MACOSX)
}  // namespace

// TODO(estade): need to port MockPrinter to get this on Linux. This involves
// hooking up Cairo to read a pdf stream, or accessing the cairo surface in the
// metafile directly.
// Same for printing via PDF on Windows.
#if (defined(OS_WIN) && !WIN_PDF_METAFILE_FOR_PRINTING) || defined(OS_MACOSX)
TEST_F(PrintWebViewHelperTest, PrintLayoutTest) {
  bool baseline = false;

  EXPECT_TRUE(chrome_render_thread_->printer() != NULL);
  for (size_t i = 0; i < arraysize(kTestPages); ++i) {
    // Load an HTML page and print it.
    LoadHTML(kTestPages[i].page);
    OnPrintPages();
    VerifyPagesPrinted(true);

    // MockRenderThread::Send() just calls MockRenderThread::OnReceived().
    // So, all IPC messages sent in the above RenderView::OnPrintPages() call
    // has been handled by the MockPrinter object, i.e. this printing job
    // has been already finished.
    // So, we can start checking the output pages of this printing job.
    // Retrieve the number of pages actually printed.
    size_t pages = chrome_render_thread_->printer()->GetPrintedPages();
    EXPECT_EQ(kTestPages[i].printed_pages, pages);

    // Retrieve the width and height of the output page.
    int width = chrome_render_thread_->printer()->GetWidth(0);
    int height = chrome_render_thread_->printer()->GetHeight(0);

    // Check with margin for error.  This has been failing with a one pixel
    // offset on our buildbot.
    const int kErrorMargin = 5;  // 5%
    EXPECT_GT(kTestPages[i].width * (100 + kErrorMargin) / 100, width);
    EXPECT_LT(kTestPages[i].width * (100 - kErrorMargin) / 100, width);
    EXPECT_GT(kTestPages[i].height * (100 + kErrorMargin) / 100, height);
    EXPECT_LT(kTestPages[i].height* (100 - kErrorMargin) / 100, height);

    // Retrieve the checksum of the bitmap data from the pseudo printer and
    // compare it with the expected result.
    std::string bitmap_actual;
    EXPECT_TRUE(
        chrome_render_thread_->printer()->GetBitmapChecksum(0, &bitmap_actual));
    if (kTestPages[i].checksum)
      EXPECT_EQ(kTestPages[i].checksum, bitmap_actual);

    if (baseline) {
      // Save the source data and the bitmap data into temporary files to
      // create base-line results.
      base::FilePath source_path;
      base::CreateTemporaryFile(&source_path);
      chrome_render_thread_->printer()->SaveSource(0, source_path);

      base::FilePath bitmap_path;
      base::CreateTemporaryFile(&bitmap_path);
      chrome_render_thread_->printer()->SaveBitmap(0, bitmap_path);
    }
  }
}
#endif

// These print preview tests do not work on Chrome OS yet.
#if !defined(OS_CHROMEOS)
class PrintWebViewHelperPreviewTest : public PrintWebViewHelperTestBase {
 public:
  PrintWebViewHelperPreviewTest() {}
  virtual ~PrintWebViewHelperPreviewTest() {}

 protected:
  void VerifyPrintPreviewCancelled(bool did_cancel) {
    bool print_preview_cancelled =
        (render_thread_->sink().GetUniqueMessageMatching(
            PrintHostMsg_PrintPreviewCancelled::ID) != NULL);
    EXPECT_EQ(did_cancel, print_preview_cancelled);
  }

  void VerifyPrintPreviewFailed(bool did_fail) {
    bool print_preview_failed =
        (render_thread_->sink().GetUniqueMessageMatching(
            PrintHostMsg_PrintPreviewFailed::ID) != NULL);
    EXPECT_EQ(did_fail, print_preview_failed);
  }

  void VerifyPrintPreviewGenerated(bool generated_preview) {
    const IPC::Message* preview_msg =
        render_thread_->sink().GetUniqueMessageMatching(
            PrintHostMsg_MetafileReadyForPrinting::ID);
    bool did_get_preview_msg = (NULL != preview_msg);
    ASSERT_EQ(generated_preview, did_get_preview_msg);
    if (did_get_preview_msg) {
      PrintHostMsg_MetafileReadyForPrinting::Param preview_param;
      PrintHostMsg_MetafileReadyForPrinting::Read(preview_msg, &preview_param);
      EXPECT_NE(0, preview_param.a.document_cookie);
      EXPECT_NE(0, preview_param.a.expected_pages_count);
      EXPECT_NE(0U, preview_param.a.data_size);
    }
  }

  void VerifyPrintFailed(bool did_fail) {
    bool print_failed = (render_thread_->sink().GetUniqueMessageMatching(
        PrintHostMsg_PrintingFailed::ID) != NULL);
    EXPECT_EQ(did_fail, print_failed);
  }

  void VerifyPrintPreviewInvalidPrinterSettings(bool settings_invalid) {
    bool print_preview_invalid_printer_settings =
        (render_thread_->sink().GetUniqueMessageMatching(
            PrintHostMsg_PrintPreviewInvalidPrinterSettings::ID) != NULL);
    EXPECT_EQ(settings_invalid, print_preview_invalid_printer_settings);
  }

  // |page_number| is 0-based.
  void VerifyDidPreviewPage(bool generate_draft_pages, int page_number) {
    bool msg_found = false;
    size_t msg_count = render_thread_->sink().message_count();
    for (size_t i = 0; i < msg_count; ++i) {
      const IPC::Message* msg = render_thread_->sink().GetMessageAt(i);
      if (msg->type() == PrintHostMsg_DidPreviewPage::ID) {
        PrintHostMsg_DidPreviewPage::Param page_param;
        PrintHostMsg_DidPreviewPage::Read(msg, &page_param);
        if (page_param.a.page_number == page_number) {
          msg_found = true;
          if (generate_draft_pages)
            EXPECT_NE(0U, page_param.a.data_size);
          else
            EXPECT_EQ(0U, page_param.a.data_size);
          break;
        }
      }
    }
    ASSERT_EQ(generate_draft_pages, msg_found);
  }

  void VerifyDefaultPageLayout(int content_width, int content_height,
                               int margin_top, int margin_bottom,
                               int margin_left, int margin_right,
                               bool page_has_print_css) {
    const IPC::Message* default_page_layout_msg =
        render_thread_->sink().GetUniqueMessageMatching(
            PrintHostMsg_DidGetDefaultPageLayout::ID);
    bool did_get_default_page_layout_msg = (NULL != default_page_layout_msg);
    if (did_get_default_page_layout_msg) {
      PrintHostMsg_DidGetDefaultPageLayout::Param param;
      PrintHostMsg_DidGetDefaultPageLayout::Read(default_page_layout_msg,
                                                 &param);
      EXPECT_EQ(content_width, param.a.content_width);
      EXPECT_EQ(content_height, param.a.content_height);
      EXPECT_EQ(margin_top, param.a.margin_top);
      EXPECT_EQ(margin_right, param.a.margin_right);
      EXPECT_EQ(margin_left, param.a.margin_left);
      EXPECT_EQ(margin_bottom, param.a.margin_bottom);
      EXPECT_EQ(page_has_print_css, param.c);
    }
  }

  DISALLOW_COPY_AND_ASSIGN(PrintWebViewHelperPreviewTest);
};

TEST_F(PrintWebViewHelperPreviewTest, BlockScriptInitiatedPrinting) {
  PrintWebViewHelper* print_web_view_helper = PrintWebViewHelper::Get(view_);
  print_web_view_helper->SetScriptedPrintBlocked(true);
  PrintWithJavaScript();
  VerifyPreviewRequest(false);

  print_web_view_helper->SetScriptedPrintBlocked(false);
  PrintWithJavaScript();
  VerifyPreviewRequest(true);
}

TEST_F(PrintWebViewHelperPreviewTest, PrintWithJavaScript) {
  LoadHTML(kPrintOnUserAction);
  gfx::Size new_size(200, 100);
  Resize(new_size, gfx::Rect(), false);

  gfx::Rect bounds = GetElementBounds("print");
  EXPECT_FALSE(bounds.IsEmpty());
  blink::WebMouseEvent mouse_event;
  mouse_event.type = blink::WebInputEvent::MouseDown;
  mouse_event.button = blink::WebMouseEvent::ButtonLeft;
  mouse_event.x = bounds.CenterPoint().x();
  mouse_event.y = bounds.CenterPoint().y();
  mouse_event.clickCount = 1;
  SendWebMouseEvent(mouse_event);
  mouse_event.type = blink::WebInputEvent::MouseUp;
  SendWebMouseEvent(mouse_event);

  VerifyPreviewRequest(true);
}

// Tests that print preview work and sending and receiving messages through
// that channel all works.
TEST_F(PrintWebViewHelperPreviewTest, OnPrintPreview) {
  LoadHTML(kHelloWorldHTML);

  // Fill in some dummy values.
  base::DictionaryValue dict;
  CreatePrintSettingsDictionary(&dict);
  OnPrintPreview(dict);

  EXPECT_EQ(0, chrome_render_thread_->print_preview_pages_remaining());
  VerifyDefaultPageLayout(540, 720, 36, 36, 36, 36, false);
  VerifyPrintPreviewCancelled(false);
  VerifyPrintPreviewFailed(false);
  VerifyPrintPreviewGenerated(true);
  VerifyPagesPrinted(false);
}

TEST_F(PrintWebViewHelperPreviewTest, PrintPreviewHTMLWithPageMarginsCss) {
  // A simple web page with print margins css.
  const char kHTMLWithPageMarginsCss[] =
      "<html><head><style>"
      "@media print {"
      "  @page {"
      "     margin: 3in 1in 2in 0.3in;"
      "  }"
      "}"
      "</style></head>"
      "<body>Lorem Ipsum:"
      "</body></html>";
  LoadHTML(kHTMLWithPageMarginsCss);

  // Fill in some dummy values.
  base::DictionaryValue dict;
  CreatePrintSettingsDictionary(&dict);
  dict.SetBoolean(kSettingPrintToPDF, false);
  dict.SetInteger(kSettingMarginsType, DEFAULT_MARGINS);
  OnPrintPreview(dict);

  EXPECT_EQ(0, chrome_render_thread_->print_preview_pages_remaining());
  VerifyDefaultPageLayout(519, 432, 216, 144, 21, 72, false);
  VerifyPrintPreviewCancelled(false);
  VerifyPrintPreviewFailed(false);
  VerifyPrintPreviewGenerated(true);
  VerifyPagesPrinted(false);
}

// Test to verify that print preview ignores print media css when non-default
// margin is selected.
TEST_F(PrintWebViewHelperPreviewTest, NonDefaultMarginsSelectedIgnorePrintCss) {
  LoadHTML(kHTMLWithPageSizeCss);

  // Fill in some dummy values.
  base::DictionaryValue dict;
  CreatePrintSettingsDictionary(&dict);
  dict.SetBoolean(kSettingPrintToPDF, false);
  dict.SetInteger(kSettingMarginsType, NO_MARGINS);
  OnPrintPreview(dict);

  EXPECT_EQ(0, chrome_render_thread_->print_preview_pages_remaining());
  VerifyDefaultPageLayout(612, 792, 0, 0, 0, 0, true);
  VerifyPrintPreviewCancelled(false);
  VerifyPrintPreviewFailed(false);
  VerifyPrintPreviewGenerated(true);
  VerifyPagesPrinted(false);
}

// Test to verify that print preview honor print media size css when
// PRINT_TO_PDF is selected and doesn't fit to printer default paper size.
TEST_F(PrintWebViewHelperPreviewTest, PrintToPDFSelectedHonorPrintCss) {
  LoadHTML(kHTMLWithPageSizeCss);

  // Fill in some dummy values.
  base::DictionaryValue dict;
  CreatePrintSettingsDictionary(&dict);
  dict.SetBoolean(kSettingPrintToPDF, true);
  dict.SetInteger(kSettingMarginsType,
                  PRINTABLE_AREA_MARGINS);
  OnPrintPreview(dict);

  EXPECT_EQ(0, chrome_render_thread_->print_preview_pages_remaining());
  // Since PRINT_TO_PDF is selected, pdf page size is equal to print media page
  // size.
  VerifyDefaultPageLayout(252, 252, 18, 18, 18, 18, true);
  VerifyPrintPreviewCancelled(false);
  VerifyPrintPreviewFailed(false);
}

// Test to verify that print preview honor print margin css when PRINT_TO_PDF
// is selected and doesn't fit to printer default paper size.
TEST_F(PrintWebViewHelperPreviewTest, PrintToPDFSelectedHonorPageMarginsCss) {
  // A simple web page with print margins css.
  const char kHTMLWithPageCss[] =
      "<html><head><style>"
      "@media print {"
      "  @page {"
      "     margin: 3in 1in 2in 0.3in;"
      "     size: 14in 14in;"
      "  }"
      "}"
      "</style></head>"
      "<body>Lorem Ipsum:"
      "</body></html>";
  LoadHTML(kHTMLWithPageCss);

  // Fill in some dummy values.
  base::DictionaryValue dict;
  CreatePrintSettingsDictionary(&dict);
  dict.SetBoolean(kSettingPrintToPDF, true);
  dict.SetInteger(kSettingMarginsType, DEFAULT_MARGINS);
  OnPrintPreview(dict);

  EXPECT_EQ(0, chrome_render_thread_->print_preview_pages_remaining());
  // Since PRINT_TO_PDF is selected, pdf page size is equal to print media page
  // size.
  VerifyDefaultPageLayout(915, 648, 216, 144, 21, 72, true);
  VerifyPrintPreviewCancelled(false);
  VerifyPrintPreviewFailed(false);
}

// Test to verify that print preview workflow center the html page contents to
// fit the page size.
TEST_F(PrintWebViewHelperPreviewTest, PrintPreviewCenterToFitPage) {
  LoadHTML(kHTMLWithPageSizeCss);

  // Fill in some dummy values.
  base::DictionaryValue dict;
  CreatePrintSettingsDictionary(&dict);
  dict.SetBoolean(kSettingPrintToPDF, false);
  dict.SetInteger(kSettingMarginsType, DEFAULT_MARGINS);
  OnPrintPreview(dict);

  EXPECT_EQ(0, chrome_render_thread_->print_preview_pages_remaining());
  VerifyDefaultPageLayout(216, 216, 288, 288, 198, 198, true);
  VerifyPrintPreviewCancelled(false);
  VerifyPrintPreviewFailed(false);
  VerifyPrintPreviewGenerated(true);
}

// Test to verify that print preview workflow scale the html page contents to
// fit the page size.
TEST_F(PrintWebViewHelperPreviewTest, PrintPreviewShrinkToFitPage) {
  // A simple web page with print margins css.
  const char kHTMLWithPageCss[] =
      "<html><head><style>"
      "@media print {"
      "  @page {"
      "     size: 15in 17in;"
      "  }"
      "}"
      "</style></head>"
      "<body>Lorem Ipsum:"
      "</body></html>";
  LoadHTML(kHTMLWithPageCss);

  // Fill in some dummy values.
  base::DictionaryValue dict;
  CreatePrintSettingsDictionary(&dict);
  dict.SetBoolean(kSettingPrintToPDF, false);
  dict.SetInteger(kSettingMarginsType, DEFAULT_MARGINS);
  OnPrintPreview(dict);

  EXPECT_EQ(0, chrome_render_thread_->print_preview_pages_remaining());
  VerifyDefaultPageLayout(571, 652, 69, 71, 20, 21, true);
  VerifyPrintPreviewCancelled(false);
  VerifyPrintPreviewFailed(false);
}

// Test to verify that print preview workflow honor the orientation settings
// specified in css.
TEST_F(PrintWebViewHelperPreviewTest, PrintPreviewHonorsOrientationCss) {
  LoadHTML(kHTMLWithLandscapePageCss);

  // Fill in some dummy values.
  base::DictionaryValue dict;
  CreatePrintSettingsDictionary(&dict);
  dict.SetBoolean(kSettingPrintToPDF, false);
  dict.SetInteger(kSettingMarginsType, NO_MARGINS);
  OnPrintPreview(dict);

  EXPECT_EQ(0, chrome_render_thread_->print_preview_pages_remaining());
  VerifyDefaultPageLayout(792, 612, 0, 0, 0, 0, true);
  VerifyPrintPreviewCancelled(false);
  VerifyPrintPreviewFailed(false);
}

// Test to verify that print preview workflow honors the orientation settings
// specified in css when PRINT_TO_PDF is selected.
TEST_F(PrintWebViewHelperPreviewTest, PrintToPDFSelectedHonorOrientationCss) {
  LoadHTML(kHTMLWithLandscapePageCss);

  // Fill in some dummy values.
  base::DictionaryValue dict;
  CreatePrintSettingsDictionary(&dict);
  dict.SetBoolean(kSettingPrintToPDF, true);
  dict.SetInteger(kSettingMarginsType, CUSTOM_MARGINS);
  OnPrintPreview(dict);

  EXPECT_EQ(0, chrome_render_thread_->print_preview_pages_remaining());
  VerifyDefaultPageLayout(748, 568, 21, 23, 21, 23, true);
  VerifyPrintPreviewCancelled(false);
  VerifyPrintPreviewFailed(false);
}

// Test to verify that complete metafile is generated for a subset of pages
// without creating draft pages.
TEST_F(PrintWebViewHelperPreviewTest, OnPrintPreviewForSelectedPages) {
  LoadHTML(kMultipageHTML);

  // Fill in some dummy values.
  base::DictionaryValue dict;
  CreatePrintSettingsDictionary(&dict);

  // Set a page range and update the dictionary to generate only the complete
  // metafile with the selected pages. Page numbers used in the dictionary
  // are 1-based.
  base::DictionaryValue* page_range = new base::DictionaryValue();
  page_range->SetInteger(kSettingPageRangeFrom, 2);
  page_range->SetInteger(kSettingPageRangeTo, 3);

  base::ListValue* page_range_array = new base::ListValue();
  page_range_array->Append(page_range);

  dict.Set(kSettingPageRange, page_range_array);
  dict.SetBoolean(kSettingGenerateDraftData, false);

  OnPrintPreview(dict);

  VerifyDidPreviewPage(false, 0);
  VerifyDidPreviewPage(false, 1);
  VerifyDidPreviewPage(false, 2);
  VerifyPreviewPageCount(3);
  VerifyPrintPreviewCancelled(false);
  VerifyPrintPreviewFailed(false);
  VerifyPrintPreviewGenerated(true);
  VerifyPagesPrinted(false);
}

// Test to verify that preview generated only for one page.
TEST_F(PrintWebViewHelperPreviewTest, OnPrintPreviewForSelectedText) {
  LoadHTML(kMultipageHTML);
  GetMainFrame()->selectRange(
      blink::WebRange::fromDocumentRange(GetMainFrame(), 1, 3));

  // Fill in some dummy values.
  base::DictionaryValue dict;
  CreatePrintSettingsDictionary(&dict);
  dict.SetBoolean(kSettingShouldPrintSelectionOnly, true);

  OnPrintPreview(dict);

  VerifyPreviewPageCount(1);
  VerifyPrintPreviewCancelled(false);
  VerifyPrintPreviewFailed(false);
  VerifyPrintPreviewGenerated(true);
  VerifyPagesPrinted(false);
}

// Tests that print preview fails and receiving error messages through
// that channel all works.
TEST_F(PrintWebViewHelperPreviewTest, OnPrintPreviewFail) {
  LoadHTML(kHelloWorldHTML);

  // An empty dictionary should fail.
  base::DictionaryValue empty_dict;
  OnPrintPreview(empty_dict);

  EXPECT_EQ(0, chrome_render_thread_->print_preview_pages_remaining());
  VerifyPrintPreviewCancelled(false);
  VerifyPrintPreviewFailed(true);
  VerifyPrintPreviewGenerated(false);
  VerifyPagesPrinted(false);
}

// Tests that cancelling print preview works.
TEST_F(PrintWebViewHelperPreviewTest, OnPrintPreviewCancel) {
  LoadHTML(kLongPageHTML);

  const int kCancelPage = 3;
  chrome_render_thread_->set_print_preview_cancel_page_number(kCancelPage);
  // Fill in some dummy values.
  base::DictionaryValue dict;
  CreatePrintSettingsDictionary(&dict);
  OnPrintPreview(dict);

  EXPECT_EQ(kCancelPage,
            chrome_render_thread_->print_preview_pages_remaining());
  VerifyPrintPreviewCancelled(true);
  VerifyPrintPreviewFailed(false);
  VerifyPrintPreviewGenerated(false);
  VerifyPagesPrinted(false);
}

// Tests that printing from print preview works and sending and receiving
// messages through that channel all works.
TEST_F(PrintWebViewHelperPreviewTest, OnPrintForPrintPreview) {
  LoadHTML(kPrintPreviewHTML);

  // Fill in some dummy values.
  base::DictionaryValue dict;
  CreatePrintSettingsDictionary(&dict);
  OnPrintForPrintPreview(dict);

  VerifyPrintFailed(false);
  VerifyPagesPrinted(true);
}

// Tests that printing from print preview fails and receiving error messages
// through that channel all works.
TEST_F(PrintWebViewHelperPreviewTest, OnPrintForPrintPreviewFail) {
  LoadHTML(kPrintPreviewHTML);

  // An empty dictionary should fail.
  base::DictionaryValue empty_dict;
  OnPrintForPrintPreview(empty_dict);

  VerifyPagesPrinted(false);
}

// Tests that when default printer has invalid printer settings, print preview
// receives error message.
TEST_F(PrintWebViewHelperPreviewTest,
       OnPrintPreviewUsingInvalidPrinterSettings) {
  LoadHTML(kPrintPreviewHTML);

  // Set mock printer to provide invalid settings.
  chrome_render_thread_->printer()->UseInvalidSettings();

  // Fill in some dummy values.
  base::DictionaryValue dict;
  CreatePrintSettingsDictionary(&dict);
  OnPrintPreview(dict);

  // We should have received invalid printer settings from |printer_|.
  VerifyPrintPreviewInvalidPrinterSettings(true);
  EXPECT_EQ(0, chrome_render_thread_->print_preview_pages_remaining());

  // It should receive the invalid printer settings message only.
  VerifyPrintPreviewFailed(false);
  VerifyPrintPreviewGenerated(false);
}

// Tests that when the selected printer has invalid page settings, print preview
// receives error message.
TEST_F(PrintWebViewHelperPreviewTest,
       OnPrintPreviewUsingInvalidPageSize) {
  LoadHTML(kPrintPreviewHTML);

  chrome_render_thread_->printer()->UseInvalidPageSize();

  base::DictionaryValue dict;
  CreatePrintSettingsDictionary(&dict);
  OnPrintPreview(dict);

  VerifyPrintPreviewInvalidPrinterSettings(true);
  EXPECT_EQ(0, chrome_render_thread_->print_preview_pages_remaining());

  // It should receive the invalid printer settings message only.
  VerifyPrintPreviewFailed(false);
  VerifyPrintPreviewGenerated(false);
}

// Tests that when the selected printer has invalid content settings, print
// preview receives error message.
TEST_F(PrintWebViewHelperPreviewTest,
       OnPrintPreviewUsingInvalidContentSize) {
  LoadHTML(kPrintPreviewHTML);

  chrome_render_thread_->printer()->UseInvalidContentSize();

  base::DictionaryValue dict;
  CreatePrintSettingsDictionary(&dict);
  OnPrintPreview(dict);

  VerifyPrintPreviewInvalidPrinterSettings(true);
  EXPECT_EQ(0, chrome_render_thread_->print_preview_pages_remaining());

  // It should receive the invalid printer settings message only.
  VerifyPrintPreviewFailed(false);
  VerifyPrintPreviewGenerated(false);
}

TEST_F(PrintWebViewHelperPreviewTest,
       OnPrintForPrintPreviewUsingInvalidPrinterSettings) {
  LoadHTML(kPrintPreviewHTML);

  // Set mock printer to provide invalid settings.
  chrome_render_thread_->printer()->UseInvalidSettings();

  // Fill in some dummy values.
  base::DictionaryValue dict;
  CreatePrintSettingsDictionary(&dict);
  OnPrintForPrintPreview(dict);

  VerifyPrintFailed(true);
  VerifyPagesPrinted(false);
}

#endif  // !defined(OS_CHROMEOS)

}  // namespace printing
