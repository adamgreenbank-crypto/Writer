#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>

#include <dds/DCPS/StaticIncludes.h>

#include "CounterTypeSupportImpl.h"

#include <ace/Log_Msg.h>

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

  DDS::DomainParticipant_var participant =
      dpf->create_participant(1,
                              PARTICIPANT_QOS_DEFAULT,
                              0,
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!participant) {
    ACE_ERROR_RETURN((LM_ERROR, "Failed to create participant.\n"), 1);
  }

  Example::CounterTypeSupport_var type_support = new Example::CounterTypeSupportImpl();

  if (type_support->register_type(participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR, "Failed to register type.\n"), 1);
  }

  CORBA::String_var type_name = type_support->get_type_name();

  DDS::Topic_var topic = participant->create_topic(
      "Counter",
      type_name,
      TOPIC_QOS_DEFAULT,
      0,
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!topic) {
    ACE_ERROR_RETURN((LM_ERROR, "Failed to create topic.\n"), 1);
  }

  DDS::Subscriber_var subscriber = participant->create_subscriber(
      SUBSCRIBER_QOS_DEFAULT,
      0,
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!subscriber) {
    ACE_ERROR_RETURN((LM_ERROR, "Failed to create subscriber.\n"), 1);
  }

  DDS::DataReader_var reader = subscriber->create_datareader(
      topic,
      DATAREADER_QOS_DEFAULT,
      0,
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!reader) {
    ACE_ERROR_RETURN((LM_ERROR, "Failed to create data reader.\n"), 1);
  }

  ACE_DEBUG((LM_INFO, "Waiting for Counter samples...\n"));

  DDS::StatusCondition_var status_condition = reader->get_statuscondition();
  status_condition->set_enabled_statuses(DDS::DATA_AVAILABLE_STATUS);

  DDS::WaitSet_var wait_set = new DDS::WaitSet();
  wait_set->attach_condition(status_condition);

  Example::CounterDataReader_var counter_reader =
      Example::CounterDataReader::_narrow(reader);

  if (!counter_reader) {
    ACE_ERROR_RETURN((LM_ERROR, "Failed to narrow CounterDataReader.\n"), 1);
  }

  while (true) {
    DDS::ConditionSeq conditions;
    DDS::Duration_t timeout = {DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};

    if (wait_set->wait(conditions, timeout) != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "WaitSet failed.\n"));
      break;
    }

    Example::Counter counter;
    DDS::SampleInfo info;

    DDS::ReturnCode_t result = counter_reader->take_next_sample(counter, info);
    while (result == DDS::RETCODE_OK) {
      if (info.valid_data) {
        ACE_DEBUG((LM_INFO, "Counter sample received: %d\n", counter.value));
      }
      result = counter_reader->take_next_sample(counter, info);
    }
  }

  participant->delete_contained_entities();
  dpf->delete_participant(participant);
  TheServiceParticipant->shutdown();

  return 0;
}
