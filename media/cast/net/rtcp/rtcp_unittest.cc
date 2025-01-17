// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "base/test/simple_test_tick_clock.h"
#include "media/cast/cast_defines.h"
#include "media/cast/net/cast_transport_config.h"
#include "media/cast/net/cast_transport_sender_impl.h"
#include "media/cast/net/pacing/paced_sender.h"
#include "media/cast/net/rtcp/rtcp.h"
#include "media/cast/net/rtcp/test_rtcp_packet_builder.h"
#include "media/cast/test/fake_single_thread_task_runner.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace media {
namespace cast {

using testing::_;

static const uint32 kSenderSsrc = 0x10203;
static const uint32 kReceiverSsrc = 0x40506;
static const int64 kAddedDelay = 123;
static const int64 kAddedShortDelay = 100;

class RtcpTestPacketSender : public PacketSender {
 public:
  explicit RtcpTestPacketSender(base::SimpleTestTickClock* testing_clock)
      : drop_packets_(false),
        short_delay_(false),
        rtcp_receiver_(NULL),
        testing_clock_(testing_clock) {}
  virtual ~RtcpTestPacketSender() {}
  // Packet lists imply a RTP packet.
  void set_rtcp_receiver(Rtcp* rtcp) { rtcp_receiver_ = rtcp; }

  void set_short_delay() { short_delay_ = true; }

  void set_drop_packets(bool drop_packets) { drop_packets_ = drop_packets; }

  // A singular packet implies a RTCP packet.
  virtual bool SendPacket(PacketRef packet,
                          const base::Closure& cb) OVERRIDE {
    if (short_delay_) {
      testing_clock_->Advance(
          base::TimeDelta::FromMilliseconds(kAddedShortDelay));
    } else {
      testing_clock_->Advance(base::TimeDelta::FromMilliseconds(kAddedDelay));
    }
    if (drop_packets_)
      return true;

    rtcp_receiver_->IncomingRtcpPacket(&packet->data[0], packet->data.size());
    return true;
  }

 private:
  bool drop_packets_;
  bool short_delay_;
  Rtcp* rtcp_receiver_;
  base::SimpleTestTickClock* testing_clock_;

  DISALLOW_COPY_AND_ASSIGN(RtcpTestPacketSender);
};

class LocalRtcpTransport : public PacedPacketSender {
 public:
  explicit LocalRtcpTransport(base::SimpleTestTickClock* testing_clock)
      : drop_packets_(false),
        short_delay_(false),
        testing_clock_(testing_clock) {}

  void set_rtcp_receiver(Rtcp* rtcp) { rtcp_ = rtcp; }

  void set_short_delay() { short_delay_ = true; }

  void set_drop_packets(bool drop_packets) { drop_packets_ = drop_packets; }

  virtual bool SendRtcpPacket(uint32 ssrc,
                              PacketRef packet) OVERRIDE {
    if (short_delay_) {
      testing_clock_->Advance(
          base::TimeDelta::FromMilliseconds(kAddedShortDelay));
    } else {
      testing_clock_->Advance(base::TimeDelta::FromMilliseconds(kAddedDelay));
    }
    if (drop_packets_)
      return true;

    rtcp_->IncomingRtcpPacket(&packet->data[0], packet->data.size());
    return true;
  }

  virtual bool SendPackets(
      const SendPacketVector& packets) OVERRIDE {
    return false;
  }

  virtual bool ResendPackets(
      const SendPacketVector& packets, const DedupInfo& dedup_info) OVERRIDE {
    return false;
  }

  virtual void CancelSendingPacket(
      const PacketKey& packet_key) OVERRIDE {
  }

 private:
  bool drop_packets_;
  bool short_delay_;
  Rtcp* rtcp_;
  base::SimpleTestTickClock* testing_clock_;

  DISALLOW_COPY_AND_ASSIGN(LocalRtcpTransport);
};

class MockReceiverStats : public RtpReceiverStatistics {
 public:
  MockReceiverStats() {}
  virtual ~MockReceiverStats() {}

  virtual void GetStatistics(uint8* fraction_lost,
                             uint32* cumulative_lost,
                             uint32* extended_high_sequence_number,
                             uint32* jitter) OVERRIDE {
    *fraction_lost = 0;
    *cumulative_lost = 0;
    *extended_high_sequence_number = 0;
    *jitter = 0;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockReceiverStats);
};

class MockFrameSender {
 public:
  MockFrameSender() {}
  virtual ~MockFrameSender() {}

  MOCK_METHOD1(OnReceivedCastFeedback,
               void(const RtcpCastMessage& cast_message));
  MOCK_METHOD4(OnReceivedRtt,
               void(base::TimeDelta rtt,
                    base::TimeDelta avg_rtt,
                    base::TimeDelta min_rtt,
                    base::TimeDelta max_rtt));
 private:
  DISALLOW_COPY_AND_ASSIGN(MockFrameSender);
};

class RtcpTest : public ::testing::Test {
 protected:
  RtcpTest()
      : testing_clock_(new base::SimpleTestTickClock()),
        task_runner_(new test::FakeSingleThreadTaskRunner(testing_clock_)),
        sender_to_receiver_(testing_clock_),
        receiver_to_sender_(testing_clock_) {
    testing_clock_->Advance(base::TimeTicks::Now() - base::TimeTicks());
  }

  virtual ~RtcpTest() {}

  static void UpdateCastTransportStatus(CastTransportStatus status) {
    bool result = (status == TRANSPORT_AUDIO_INITIALIZED ||
                   status == TRANSPORT_VIDEO_INITIALIZED);
    EXPECT_TRUE(result);
  }

  void RunTasks(int during_ms) {
    for (int i = 0; i < during_ms; ++i) {
      // Call process the timers every 1 ms.
      testing_clock_->Advance(base::TimeDelta::FromMilliseconds(1));
      task_runner_->RunTasks();
    }
  }

  base::SimpleTestTickClock* testing_clock_;  // Owned by CastEnvironment.
  scoped_refptr<test::FakeSingleThreadTaskRunner> task_runner_;
  LocalRtcpTransport sender_to_receiver_;
  LocalRtcpTransport receiver_to_sender_;
  MockFrameSender mock_frame_sender_;
  MockReceiverStats stats_;

  DISALLOW_COPY_AND_ASSIGN(RtcpTest);
};

TEST_F(RtcpTest, BasicSenderReport) {
  Rtcp rtcp(base::Bind(&MockFrameSender::OnReceivedCastFeedback,
                       base::Unretained(&mock_frame_sender_)),
            base::Bind(&MockFrameSender::OnReceivedRtt,
                       base::Unretained(&mock_frame_sender_)),
            RtcpLogMessageCallback(),
            testing_clock_,
            &sender_to_receiver_,
            kSenderSsrc,
            kReceiverSsrc);
  sender_to_receiver_.set_rtcp_receiver(&rtcp);
  rtcp.SendRtcpFromRtpSender(base::TimeTicks(), 0, 1, 1);
}

TEST_F(RtcpTest, BasicReceiverReport) {
  Rtcp rtcp(base::Bind(&MockFrameSender::OnReceivedCastFeedback,
                       base::Unretained(&mock_frame_sender_)),
            base::Bind(&MockFrameSender::OnReceivedRtt,
                       base::Unretained(&mock_frame_sender_)),
            RtcpLogMessageCallback(),
            testing_clock_,
            &receiver_to_sender_,
            kSenderSsrc,
            kReceiverSsrc);
  receiver_to_sender_.set_rtcp_receiver(&rtcp);
  rtcp.SendRtcpFromRtpReceiver(NULL, base::TimeDelta(), NULL, &stats_);
}

TEST_F(RtcpTest, BasicCast) {
  EXPECT_CALL(mock_frame_sender_, OnReceivedCastFeedback(_)).Times(1);

  // Media sender.
  Rtcp rtcp(base::Bind(&MockFrameSender::OnReceivedCastFeedback,
                       base::Unretained(&mock_frame_sender_)),
            base::Bind(&MockFrameSender::OnReceivedRtt,
                       base::Unretained(&mock_frame_sender_)),
            RtcpLogMessageCallback(),
            testing_clock_,
            &receiver_to_sender_,
            kSenderSsrc,
            kSenderSsrc);
  receiver_to_sender_.set_rtcp_receiver(&rtcp);
  RtcpCastMessage cast_message(kSenderSsrc);
  cast_message.ack_frame_id = kAckFrameId;
  PacketIdSet missing_packets;
  cast_message.missing_frames_and_packets[kLostFrameId] = missing_packets;

  missing_packets.insert(kLostPacketId1);
  missing_packets.insert(kLostPacketId2);
  missing_packets.insert(kLostPacketId3);
  cast_message.missing_frames_and_packets[kFrameIdWithLostPackets] =
      missing_packets;
  rtcp.SendRtcpFromRtpReceiver(&cast_message, base::TimeDelta(), NULL, NULL);
}

TEST_F(RtcpTest, RttReducedSizeRtcp) {
  // Media receiver.
  Rtcp rtcp_receiver(RtcpCastMessageCallback(),
                     RtcpRttCallback(),
                     RtcpLogMessageCallback(),
                     testing_clock_,
                     &receiver_to_sender_,
                     kReceiverSsrc,
                     kSenderSsrc);

  // Media sender.
  Rtcp rtcp_sender(base::Bind(&MockFrameSender::OnReceivedCastFeedback,
                              base::Unretained(&mock_frame_sender_)),
                   base::Bind(&MockFrameSender::OnReceivedRtt,
                              base::Unretained(&mock_frame_sender_)),
                   RtcpLogMessageCallback(),
                   testing_clock_,
                   &sender_to_receiver_,
                   kSenderSsrc,
                   kReceiverSsrc);

  sender_to_receiver_.set_rtcp_receiver(&rtcp_receiver);
  receiver_to_sender_.set_rtcp_receiver(&rtcp_sender);

  base::TimeDelta rtt;
  base::TimeDelta avg_rtt;
  base::TimeDelta min_rtt;
  base::TimeDelta max_rtt;
  EXPECT_FALSE(rtcp_sender.Rtt(&rtt, &avg_rtt, &min_rtt, &max_rtt));

  rtcp_sender.SendRtcpFromRtpSender(testing_clock_->NowTicks(), 1, 1, 1);
  RunTasks(33);
  rtcp_receiver.SendRtcpFromRtpReceiver(NULL, base::TimeDelta(), NULL, &stats_);
  EXPECT_TRUE(rtcp_sender.Rtt(&rtt, &avg_rtt, &min_rtt, &max_rtt));
  rtcp_sender.SendRtcpFromRtpSender(testing_clock_->NowTicks(), 2, 1, 1);
  RunTasks(33);
}

TEST_F(RtcpTest, Rtt) {
  // Media receiver.
  Rtcp rtcp_receiver(RtcpCastMessageCallback(),
                     RtcpRttCallback(),
                     RtcpLogMessageCallback(),
                     testing_clock_,
                     &receiver_to_sender_,
                     kReceiverSsrc,
                     kSenderSsrc);

  // Media sender.
  Rtcp rtcp_sender(base::Bind(&MockFrameSender::OnReceivedCastFeedback,
                              base::Unretained(&mock_frame_sender_)),
                   base::Bind(&MockFrameSender::OnReceivedRtt,
                              base::Unretained(&mock_frame_sender_)),
                   RtcpLogMessageCallback(),
                   testing_clock_,
                   &sender_to_receiver_,
                   kSenderSsrc,
                   kReceiverSsrc);

  receiver_to_sender_.set_rtcp_receiver(&rtcp_sender);
  sender_to_receiver_.set_rtcp_receiver(&rtcp_receiver);

  base::TimeDelta rtt;
  base::TimeDelta avg_rtt;
  base::TimeDelta min_rtt;
  base::TimeDelta max_rtt;
  EXPECT_FALSE(rtcp_sender.Rtt(&rtt, &avg_rtt, &min_rtt, &max_rtt));

  rtcp_sender.SendRtcpFromRtpSender(testing_clock_->NowTicks(), 1, 1, 1);
  RunTasks(33);
  rtcp_receiver.SendRtcpFromRtpReceiver(NULL, base::TimeDelta(), NULL, &stats_);

  EXPECT_TRUE(rtcp_sender.Rtt(&rtt, &avg_rtt, &min_rtt, &max_rtt));
  RunTasks(33);

  RunTasks(33);

  EXPECT_NEAR(2 * kAddedDelay, rtt.InMilliseconds(), 2);
  EXPECT_NEAR(2 * kAddedDelay, avg_rtt.InMilliseconds(), 2);
  EXPECT_NEAR(2 * kAddedDelay, min_rtt.InMilliseconds(), 2);
  EXPECT_NEAR(2 * kAddedDelay, max_rtt.InMilliseconds(), 2);

  rtcp_sender.SendRtcpFromRtpSender(testing_clock_->NowTicks(), 2, 1, 1);
  RunTasks(33);

  receiver_to_sender_.set_short_delay();
  sender_to_receiver_.set_short_delay();
  rtcp_receiver.SendRtcpFromRtpReceiver(NULL, base::TimeDelta(), NULL, &stats_);
  EXPECT_TRUE(rtcp_sender.Rtt(&rtt, &avg_rtt, &min_rtt, &max_rtt));
  EXPECT_NEAR(kAddedDelay + kAddedShortDelay, rtt.InMilliseconds(), 2);
  EXPECT_NEAR(
      (kAddedShortDelay + 3 * kAddedDelay) / 2, avg_rtt.InMilliseconds(), 2);
  EXPECT_NEAR(kAddedDelay + kAddedShortDelay, min_rtt.InMilliseconds(), 2);
  EXPECT_NEAR(2 * kAddedDelay, max_rtt.InMilliseconds(), 2);

  rtcp_sender.SendRtcpFromRtpSender(testing_clock_->NowTicks(), 3, 1, 1);
  RunTasks(33);

  rtcp_receiver.SendRtcpFromRtpReceiver(NULL, base::TimeDelta(), NULL, &stats_);
  EXPECT_TRUE(rtcp_sender.Rtt(&rtt, &avg_rtt, &min_rtt, &max_rtt));
  EXPECT_NEAR(2 * kAddedShortDelay, rtt.InMilliseconds(), 2);
  EXPECT_NEAR(2 * kAddedShortDelay, min_rtt.InMilliseconds(), 2);
  EXPECT_NEAR(2 * kAddedDelay, max_rtt.InMilliseconds(), 2);

  rtcp_receiver.SendRtcpFromRtpReceiver(NULL, base::TimeDelta(), NULL, &stats_);
  EXPECT_TRUE(rtcp_sender.Rtt(&rtt, &avg_rtt, &min_rtt, &max_rtt));
  EXPECT_NEAR(2 * kAddedShortDelay, rtt.InMilliseconds(), 2);
  EXPECT_NEAR(2 * kAddedShortDelay, min_rtt.InMilliseconds(), 2);
  EXPECT_NEAR(2 * kAddedDelay, max_rtt.InMilliseconds(), 2);
}

TEST_F(RtcpTest, RttWithPacketLoss) {
  // Media receiver.
  Rtcp rtcp_receiver(RtcpCastMessageCallback(),
                     RtcpRttCallback(),
                     RtcpLogMessageCallback(),
                     testing_clock_,
                     &receiver_to_sender_,
                     kReceiverSsrc,
                     kSenderSsrc);

  // Media sender.
  Rtcp rtcp_sender(base::Bind(&MockFrameSender::OnReceivedCastFeedback,
                              base::Unretained(&mock_frame_sender_)),
                   base::Bind(&MockFrameSender::OnReceivedRtt,
                              base::Unretained(&mock_frame_sender_)),
                   RtcpLogMessageCallback(),
                   testing_clock_,
                   &sender_to_receiver_,
                   kSenderSsrc,
                   kReceiverSsrc);

  receiver_to_sender_.set_rtcp_receiver(&rtcp_sender);
  sender_to_receiver_.set_rtcp_receiver(&rtcp_receiver);

  rtcp_receiver.SendRtcpFromRtpReceiver(NULL, base::TimeDelta(), NULL, &stats_);
  rtcp_sender.SendRtcpFromRtpSender(testing_clock_->NowTicks(), 0, 1, 1);
  RunTasks(33);

  base::TimeDelta rtt;
  base::TimeDelta avg_rtt;
  base::TimeDelta min_rtt;
  base::TimeDelta max_rtt;
  EXPECT_FALSE(rtcp_sender.Rtt(&rtt, &avg_rtt, &min_rtt, &max_rtt));

  receiver_to_sender_.set_short_delay();
  sender_to_receiver_.set_short_delay();
  receiver_to_sender_.set_drop_packets(true);

  rtcp_receiver.SendRtcpFromRtpReceiver(NULL, base::TimeDelta(), NULL, &stats_);
  rtcp_sender.SendRtcpFromRtpSender(testing_clock_->NowTicks(), 1, 1, 1);
  RunTasks(33);

}

TEST_F(RtcpTest, NtpAndTime) {
  const int64 kSecondsbetweenYear1900and2010 = INT64_C(40176 * 24 * 60 * 60);
  const int64 kSecondsbetweenYear1900and2030 = INT64_C(47481 * 24 * 60 * 60);

  uint32 ntp_seconds_1 = 0;
  uint32 ntp_fraction_1 = 0;
  base::TimeTicks input_time = base::TimeTicks::Now();
  ConvertTimeTicksToNtp(input_time, &ntp_seconds_1, &ntp_fraction_1);

  // Verify absolute value.
  EXPECT_GT(ntp_seconds_1, kSecondsbetweenYear1900and2010);
  EXPECT_LT(ntp_seconds_1, kSecondsbetweenYear1900and2030);

  base::TimeTicks out_1 = ConvertNtpToTimeTicks(ntp_seconds_1, ntp_fraction_1);
  EXPECT_EQ(input_time, out_1);  // Verify inverse.

  base::TimeDelta time_delta = base::TimeDelta::FromMilliseconds(1000);
  input_time += time_delta;

  uint32 ntp_seconds_2 = 0;
  uint32 ntp_fraction_2 = 0;

  ConvertTimeTicksToNtp(input_time, &ntp_seconds_2, &ntp_fraction_2);
  base::TimeTicks out_2 = ConvertNtpToTimeTicks(ntp_seconds_2, ntp_fraction_2);
  EXPECT_EQ(input_time, out_2);  // Verify inverse.

  // Verify delta.
  EXPECT_EQ((out_2 - out_1), time_delta);
  EXPECT_EQ((ntp_seconds_2 - ntp_seconds_1), UINT32_C(1));
  EXPECT_NEAR(ntp_fraction_2, ntp_fraction_1, 1);

  time_delta = base::TimeDelta::FromMilliseconds(500);
  input_time += time_delta;

  uint32 ntp_seconds_3 = 0;
  uint32 ntp_fraction_3 = 0;

  ConvertTimeTicksToNtp(input_time, &ntp_seconds_3, &ntp_fraction_3);
  base::TimeTicks out_3 = ConvertNtpToTimeTicks(ntp_seconds_3, ntp_fraction_3);
  EXPECT_EQ(input_time, out_3);  // Verify inverse.

  // Verify delta.
  EXPECT_EQ((out_3 - out_2), time_delta);
  EXPECT_NEAR((ntp_fraction_3 - ntp_fraction_2), 0xffffffff / 2, 1);
}

}  // namespace cast
}  // namespace media
