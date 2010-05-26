/*
Copyright c1997-2008 Trygve Isaacson. All rights reserved.
This file is part of the Code Vault version 3.0
http://www.bombaydigital.com/
*/

/** @file */

#include "vunit.h"

#include "vlogger.h"
#include "vexception.h"
#include "vtextiostream.h"

// VUnit ---------------------------------------------------------------------

// static
void VUnit::runUnit(VUnit& unit, VUnitOutputWriterList* writers)
    {
    unit.setWriters(writers);

    unit.logStart();

    try
        {
        unit.run();
        }
    catch (const std::exception& ex)    // will include VException
        {
        unit.logExceptionalEnd(ex.what());
        throw;
        }
    catch (...)
        {
        unit.logExceptionalEnd("(exception type unknown)");
        throw;
        }

    unit.logNormalEnd();
    }

VUnit::VUnit(const VString& name, bool logOnSuccess, bool throwOnError) :
mName(name),
mLogOnSuccess(logOnSuccess),
mThrowOnError(throwOnError),
mWriters(NULL),
mNumSuccessfulTests(0),
mNumFailedTests(0),
mResults(),
mUnitStartTimeSnapshot(VInstant::snapshot()),
mPreviousTestEndedSnapshot(VInstant::snapshot()),
mLastTestDescription()
    {
    }

VUnit::~VUnit()
    {
    }

void VUnit::logStart()
    {
    if (mWriters != NULL)
        for (VUnitOutputWriterList::iterator i = mWriters->begin(); i != mWriters->end(); ++i)
            (*i)->testSuiteBegin(mName);
    }

void VUnit::logNormalEnd()
    {
    if (mWriters != NULL)
        for (VUnitOutputWriterList::iterator i = mWriters->begin(); i != mWriters->end(); ++i)
            (*i)->testSuiteEnd();
    }

void VUnit::logExceptionalEnd(const VString& exceptionMessage)
    {
    VTestInfo error(false, VString("after %s, threw exception: %s", mLastTestDescription.chars(), exceptionMessage.chars()), VDuration::ZERO());
    mResults.push_back(error);

    ++mNumFailedTests;

    if (mWriters != NULL)
        for (VUnitOutputWriterList::iterator i = mWriters->begin(); i != mWriters->end(); ++i)
            {
            (*i)->testCaseBegin("exception thrown");
            (*i)->testCaseEnd(error);
            }
    }

void VUnit::testAssertion(bool successful, const VString& filePath, int lineNumber, const VString& labelSuffix, const VString& expectedDescription)
    {
    VString fileName;
    filePath.getSubstring(fileName, filePath.lastIndexOf('/') + 1);
    VString testName("%s:%d %s", fileName.chars(), lineNumber, labelSuffix.chars());

    mLastTestDescription = testName;

    if (successful)
        this->recordSuccess(testName);
    else
        this->recordFailure(VString("%s: %s", testName.chars(), expectedDescription.chars()));

    mPreviousTestEndedSnapshot = VInstant::snapshot();
    }

void VUnit::test(bool successful, const VString& description)
    {
    mLastTestDescription = description;

    if (successful)
        this->recordSuccess(description);
    else
        this->recordFailure(description);

    mPreviousTestEndedSnapshot = VInstant::snapshot();
    }

void VUnit::test(const VString& a, const VString& b, const VString& description)
    {
    this->test(a == b, description);
    }

void VUnit::logStatus(const VString& description)
    {
    if (mWriters != NULL)
        for (VUnitOutputWriterList::iterator i = mWriters->begin(); i != mWriters->end(); ++i)
            (*i)->testSuiteStatusMessage(description);
    }

void VUnit::logMessage(const VString& message)
    {
    // Note that we don't use VLOGGER_xxx and its time stamping and
    // formatting, but use raw logging, so that:
    // - we don't time stamp the messages, so diff will work well
    // - the code undergoing testing can log w/o us interfering and vice versa

    VLogger::getLogger("VUnit")->rawLog(message);
    }

void VUnit::recordSuccess(const VString& description)
    {
    if (mWriters != NULL)
        for (VUnitOutputWriterList::iterator i = mWriters->begin(); i != mWriters->end(); ++i)
            (*i)->testCaseBegin(description);

    ++mNumSuccessfulTests;

    VTestInfo info(true, description, VInstant::snapshotDelta(mPreviousTestEndedSnapshot));
    mResults.push_back(info);

    if (mWriters != NULL)
        for (VUnitOutputWriterList::iterator i = mWriters->begin(); i != mWriters->end(); ++i)
            (*i)->testCaseEnd(info);
    }

void VUnit::recordFailure(const VString& description)
    {
    if (mWriters != NULL)
        for (VUnitOutputWriterList::iterator i = mWriters->begin(); i != mWriters->end(); ++i)
            (*i)->testCaseBegin(description);

    ++mNumFailedTests;

    VTestInfo info(false, description, VInstant::snapshotDelta(mPreviousTestEndedSnapshot));
    mResults.push_back(info);

    if (mWriters != NULL)
        for (VUnitOutputWriterList::iterator i = mWriters->begin(); i != mWriters->end(); ++i)
            (*i)->testCaseEnd(info);
    }

// VTestInfo -----------------------------------------------------------------

VTestInfo::VTestInfo(bool success, const VString& description, const VDuration& duration) :
mSuccess(success),
mDescription(description),
mDuration(duration)
    {
    // Some tests that manipulate time simulation will yield bogus durations.
    if ((mDuration < VDuration::ZERO()) || (mDuration > VDuration::DAY()))
        mDuration = VDuration::ZERO();
    }

// VFailureEmitter -----------------------------------------------------------

VFailureEmitter::VFailureEmitter(const VString& testName, bool logOnSuccess, bool throwOnError, const VString& errorMessage) :
VUnit(testName, logOnSuccess, throwOnError), fErrorMessage(errorMessage)
    {
    }

void VFailureEmitter::run()
    {
    this->logStatus(VString("%s failed due to this error: %s", this->getName().chars(), fErrorMessage.chars()));
    this->test(false, fErrorMessage);
    }

// VUnitOutputWriter ------------------------------------------------------

static const VString VUNIT_OUTPUT_DIRECTIVE("-vunit-out");

static const VString OUTPUT_TYPE_SIMPLE("text");
static const VString OUTPUT_TYPE_JUNIT("junit");
static const VString OUTPUT_TYPE_TEAMCITY("tc");
static const VString OUTPUT_TYPE_TEAMCITY_STATUS("tcstatus");

static const VString OUTPUT_FILEPATH_STDOUT("stdout");

// static
void VUnitOutputWriter::createOutputWriters(const VStringVector& args, VUnitOutputWriterList& writers, VLoggerList& loggers)
    {
    for (VStringVector::const_iterator i = args.begin(); i != args.end(); ++i)
        {
        if ((*i) == VUNIT_OUTPUT_DIRECTIVE)
            {
            VString outputType = *(++i);
            VString filePath = *(++i);
            VUnitOutputWriter::_addNewOutputWriter(writers, loggers, outputType, filePath);
            }
        }

    // If no specific output was specified, log simple output to stdout.
    if (writers.size() == 0)
        VUnitOutputWriter::_addNewOutputWriter(writers, loggers, OUTPUT_TYPE_SIMPLE, OUTPUT_FILEPATH_STDOUT);
    }

VUnitOutputWriter::VUnitOutputWriter(VLogger& outputLogger) :
mLogger(outputLogger),
mTestSuitesStartTime(VInstant::NEVER_OCCURRED()),
mTotalNumSuccesses(0),
mTotalNumErrors(0),
mCurrentTestSuiteName(),
mCurrentTestSuiteResults(),
mCurrentTestSuiteNumSuccesses(0),
mCurrentTestSuiteNumErrors(0),
mCurrentTestSuiteStartTime(VInstant::NEVER_OCCURRED()),
mCurrentTestSuiteEndTime(VInstant::NEVER_OCCURRED()),
mCurrentTestCaseName(),
mCurrentTestCaseStartTime(VInstant::NEVER_OCCURRED()),
mCurrentTestCaseEndTime(VInstant::NEVER_OCCURRED()),
mFailedTestSuiteNames()
    {
    }

void VUnitOutputWriter::_testSuitesBegin()
    {
    mTestSuitesStartTime.setNow();
    }

void VUnitOutputWriter::_testSuiteBegin(const VString& testSuiteName)
    {
    mCurrentTestSuiteName = testSuiteName;
    mCurrentTestSuiteResults.clear();
    mCurrentTestSuiteNumSuccesses = 0;
    mCurrentTestSuiteNumErrors = 0;
    mCurrentTestSuiteStartTime.setNow();
    mCurrentTestSuiteEndTime = VInstant::NEVER_OCCURRED();
    mCurrentTestCaseStartTime = VInstant::NEVER_OCCURRED();
    mCurrentTestCaseEndTime = VInstant::NEVER_OCCURRED();
    }

void VUnitOutputWriter::_testCaseBegin(const VString& testCaseName)
    {
    mCurrentTestCaseName = testCaseName;
    mCurrentTestCaseStartTime.setNow();
    }

void VUnitOutputWriter::_testCaseEnd(const VTestInfo& testInfo)
    {
    mCurrentTestCaseEndTime.setNow();
    mCurrentTestSuiteResults.push_back(testInfo);

    if (testInfo.mSuccess)
        {
        ++mTotalNumSuccesses;
        ++mCurrentTestSuiteNumSuccesses;
        }
    else
        {
        ++mTotalNumErrors;
        ++mCurrentTestSuiteNumErrors;
        }
    }

void VUnitOutputWriter::_testSuiteEnd()
    {
    mCurrentTestSuiteEndTime.setNow();

    if (mCurrentTestSuiteNumErrors != 0)
        mFailedTestSuiteNames.push_back(mCurrentTestSuiteName);
    }

// static
VLogger* VUnitOutputWriter::_newLoggerByType(const VString& outputType, const VString& filePath)
    {
    // We allow either cout logging, or file logging.
    VLogger* logger = NULL;
    if (filePath == OUTPUT_FILEPATH_STDOUT)
        {
        logger = new VCoutLogger(VLogger::kTrace, VString("vunit-%s-cout", outputType.chars()), VString::EMPTY());
        }
    else
        {
        VFSNode logFile(filePath);
        logFile.rm();
        logger = new VFileLogger(VLogger::kTrace, VString("vunit-%s-%s", outputType.chars(), filePath.chars()), VString::EMPTY(), filePath);
        }

    return logger;
    }

// static
VUnitOutputWriter* VUnitOutputWriter::_newOutputWriterByType(const VString& outputType, VLogger* logger)
    {
    VUnitOutputWriter* writer = NULL;

    if (outputType == OUTPUT_TYPE_SIMPLE)
        {
        writer = new VUnitSimpleTextOutput(*logger);
        }
    else if (outputType == OUTPUT_TYPE_JUNIT)
        {
        writer = new VUnitJUnitXMLOutput(*logger);
        }
    else if (outputType == OUTPUT_TYPE_TEAMCITY)
        {
        writer = new VUnitTeamCityOutput(*logger);
        }
    else if (outputType == OUTPUT_TYPE_TEAMCITY_STATUS)
        {
        writer = new VUnitTeamCityBuildStatusOutput(*logger);
        }
    else
        {
        VLOGGER_ERROR(VString("Invalid unit test output type '%s' will be ignored.", outputType.chars()));
        }

    return writer;
    }

// static
void VUnitOutputWriter::_addNewOutputWriter(VUnitOutputWriterList& outputters, VLoggerList& outputLoggers, const VString& outputType, const VString& filePath)
    {
    VLogger* logger = VUnitOutputWriter::_newLoggerByType(outputType, filePath);
    VUnitOutputWriter* outputInterface = VUnitOutputWriter::_newOutputWriterByType(outputType, logger);

    if (outputInterface == NULL)
        {
        delete logger;
        }
    else
        {
        outputLoggers.push_back(logger);
        outputters.push_back(outputInterface);
        }
    }

// VTestSuitesWrapper --------------------------------------------------------

VTestSuitesWrapper::VTestSuitesWrapper(const VStringVector& args) :
mWriters(),
mLoggers()
    {
    VUnitOutputWriter::createOutputWriters(args, mWriters, mLoggers);

    for (VUnitOutputWriterList::iterator i = mWriters.begin(); i != mWriters.end(); ++i)
        (*i)->testSuitesBegin();
    }

VTestSuitesWrapper::~VTestSuitesWrapper()
    {
    for (VUnitOutputWriterList::iterator i = mWriters.begin(); i != mWriters.end(); ++i)
        (*i)->testSuitesEnd();

    vault::vectorDeleteAll(mWriters);
    vault::vectorDeleteAll(mLoggers);
    }

// VUnitJUnitXMLOutput -------------------------------------------------------

static VString _escapeXMLString(const VString& original)
    {
    VString result(original);

    result.replace("&", "&amp;");
    result.replace("\"", "&quot;");
    result.replace("<", "&lt;");
    result.replace(">", "&gt;");

    return result;
    }

VUnitJUnitXMLOutput::VUnitJUnitXMLOutput(VLogger& outputLogger) :
VUnitOutputWriter(outputLogger)
    {
    }

void VUnitJUnitXMLOutput::testSuitesBegin()
    {
    this->_testSuitesBegin();

    mLogger.rawLog("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>");
    mLogger.rawLog("<testsuites>");
    }

void VUnitJUnitXMLOutput::testSuiteBegin(const VString& testSuiteName)
    {
    this->_testSuiteBegin(testSuiteName);
    }

void VUnitJUnitXMLOutput::testSuiteStatusMessage(const VString& /*message*/)
    {
    }

void VUnitJUnitXMLOutput::testCaseBegin(const VString& testCaseName)
    {
    this->_testCaseBegin(testCaseName);
    }

void VUnitJUnitXMLOutput::testCaseEnd(const VTestInfo& testInfo)
    {
    this->_testCaseEnd(testInfo);
    }

void VUnitJUnitXMLOutput::testSuiteEnd()
    {
    this->_testSuiteEnd();

    VDuration testSuiteDuration = mCurrentTestSuiteEndTime - mCurrentTestSuiteStartTime;

    mLogger.rawLog(VString(" <testsuite errors=\"%d\" failures=\"0\" name=\"%s\" tests=\"%d\" time=\"%s\">",
        mCurrentTestSuiteNumErrors, mCurrentTestSuiteName.chars(), (int)mCurrentTestSuiteResults.size(), testSuiteDuration.getDurationString().chars()));

    for (TestInfoVector::const_iterator i = mCurrentTestSuiteResults.begin(); i != mCurrentTestSuiteResults.end(); ++i)
        {
        mLogger.rawLog(VString("  <testcase class=\"%s\" name=\"%s\" time=\"%s\"></testcase>",
            mCurrentTestSuiteName.chars(), _escapeXMLString((*i).mDescription).chars(), (*i).mDuration.getDurationString().chars()));
        }

    mLogger.rawLog(" </testsuite>");
    }

void VUnitJUnitXMLOutput::testSuitesEnd()
    {
    mLogger.rawLog("</testsuites>");
    }

// VUnitSimpleTextOutput -----------------------------------------------------

VUnitSimpleTextOutput::VUnitSimpleTextOutput(VLogger& outputLogger) :
VUnitOutputWriter(outputLogger)
    {
    }

void VUnitSimpleTextOutput::testSuitesBegin()
    {
    this->_testSuitesBegin();

    VString nowText;
    mTestSuitesStartTime.getLocalString(nowText);
    mLogger.rawLog(VString("[status ] Test run starting at %s.", nowText.chars()));
    mLogger.rawLog(VString::EMPTY());
    }

void VUnitSimpleTextOutput::testSuiteBegin(const VString& testSuiteName)
    {
    this->_testSuiteBegin(testSuiteName);

    mLogger.rawLog(VString("[status ] %s : starting.", testSuiteName.chars()));
    }

void VUnitSimpleTextOutput::testSuiteStatusMessage(const VString& message)
    {
    mLogger.rawLog(VString("[status ] %s : %s", mCurrentTestSuiteName.chars(), message.chars()));
    }

void VUnitSimpleTextOutput::testCaseBegin(const VString& testCaseName)
    {
    this->_testCaseBegin(testCaseName);
    }

void VUnitSimpleTextOutput::testCaseEnd(const VTestInfo& testInfo)
    {
    this->_testCaseEnd(testInfo);

    mLogger.rawLog(VString("[%s] %s : %s.", (testInfo.mSuccess ? "success":"FAILURE"), mCurrentTestSuiteName.chars(), testInfo.mDescription.chars()));
    }

void VUnitSimpleTextOutput::testSuiteEnd()
    {
    this->_testSuiteEnd();

    mLogger.rawLog(VString("[status ] %s : ended.", mCurrentTestSuiteName.chars()));
    mLogger.rawLog(VString("[results] %s : tests passed: %d", mCurrentTestSuiteName.chars(), mCurrentTestSuiteNumSuccesses));
    mLogger.rawLog(VString("[results] %s : tests failed: %d", mCurrentTestSuiteName.chars(), mCurrentTestSuiteNumErrors));
    mLogger.rawLog(VString("[results] %s : summary: %s.", mCurrentTestSuiteName.chars(), ((mCurrentTestSuiteNumErrors == 0) ? "success":"FAILURE")));
    mLogger.rawLog(VString::EMPTY());
    }

void VUnitSimpleTextOutput::testSuitesEnd()
    {
    mLogger.rawLog(VString("[results] TOTAL tests passed: %d", mTotalNumSuccesses));
    mLogger.rawLog(VString("[results] TOTAL tests failed: %d", mTotalNumErrors));
    mLogger.rawLog(VString("[results] TOTAL summary: %s.", ((mTotalNumErrors == 0) ? "success":"FAILURE")));

    if (mFailedTestSuiteNames.size() != 0)
        {
        VString names;
        for (VStringVector::const_iterator i = mFailedTestSuiteNames.begin(); i != mFailedTestSuiteNames.end(); ++i)
            {
            names += ' ';
            names += *i;
            }
        mLogger.rawLog(VString("[results] Names of suites with failures:%s", names.chars()));
        }

    VInstant now;
    VDuration totalTestTime = now - mTestSuitesStartTime;
    VString nowText;
    now.getLocalString(nowText);
    mLogger.rawLog(VString::EMPTY());
    mLogger.rawLog(VString("[status ] Test run ending at %s. Total time %s.", nowText.chars(), totalTestTime.getDurationString().chars()));
    }

// VUnitTeamCityOutput -------------------------------------------------------

static VString _escapeTeamCityString(const VString& original)
    {
    VString result = original;

    result.replace("|", "||");
    result.replace("'", "|'");
    result.replace("\n", "|\n");
    result.replace("\r", "|\r");
    result.replace("]", "|]");

    return result;
    }

VUnitTeamCityOutput::VUnitTeamCityOutput(VLogger& outputLogger) :
VUnitOutputWriter(outputLogger)
    {
    }

void VUnitTeamCityOutput::testSuitesBegin()
    {
    this->_testSuitesBegin();
    }

void VUnitTeamCityOutput::testSuiteBegin(const VString& testSuiteName)
    {
    this->_testSuiteBegin(testSuiteName);

    mLogger.rawLog(VString("##teamcity[testSuiteStarted name='%s']", _escapeTeamCityString(testSuiteName).chars()));
    }

void VUnitTeamCityOutput::testSuiteStatusMessage(const VString& /*message*/)
    {
    }

void VUnitTeamCityOutput::testCaseBegin(const VString& testCaseName)
    {
    this->_testCaseBegin(testCaseName);

    mLogger.rawLog(VString("##teamcity[testStarted name='%s']", _escapeTeamCityString(testCaseName).chars()));
    }

void VUnitTeamCityOutput::testCaseEnd(const VTestInfo& testInfo)
    {
    this->_testCaseEnd(testInfo);

    if (!testInfo.mSuccess)
        mLogger.rawLog(VString("##teamcity[testFailed name='%s' message='%s']", _escapeTeamCityString(mCurrentTestCaseName).chars(), _escapeTeamCityString(testInfo.mDescription).chars()));

    mLogger.rawLog(VString("##teamcity[testFinished name='%s']", _escapeTeamCityString(mCurrentTestCaseName).chars()));
    }

void VUnitTeamCityOutput::testSuiteEnd()
    {
    this->_testSuiteEnd();

    mLogger.rawLog(VString("##teamcity[testSuiteFinished name='%s']", _escapeTeamCityString(mCurrentTestSuiteName).chars()));
    }

void VUnitTeamCityOutput::testSuitesEnd()
    {
    }

// VUnitTeamCityBuildStatusOutput --------------------------------------------

VUnitTeamCityBuildStatusOutput::VUnitTeamCityBuildStatusOutput(VLogger& outputLogger) :
VUnitOutputWriter(outputLogger)
    {
    }

void VUnitTeamCityBuildStatusOutput::testSuitesBegin()
    {
    this->_testSuitesBegin();
    }

void VUnitTeamCityBuildStatusOutput::testSuiteBegin(const VString& testSuiteName)
    {
    this->_testSuiteBegin(testSuiteName);
    }

void VUnitTeamCityBuildStatusOutput::testSuiteStatusMessage(const VString& /*message*/)
    {
    }

void VUnitTeamCityBuildStatusOutput::testCaseBegin(const VString& testCaseName)
    {
    this->_testCaseBegin(testCaseName);
    }

void VUnitTeamCityBuildStatusOutput::testCaseEnd(const VTestInfo& testInfo)
    {
    this->_testCaseEnd(testInfo);
    }

void VUnitTeamCityBuildStatusOutput::testSuiteEnd()
    {
    this->_testSuiteEnd();
    }

void VUnitTeamCityBuildStatusOutput::testSuitesEnd()
    {
    try
        {
        mLogger.rawLog(VString("<build number=\"{build.number}\">"));
        mLogger.rawLog(VString(" <statusInfo status=\"%s\">", (mTotalNumErrors == 0 ? "SUCCESS" : "FAILURE")));
        mLogger.rawLog(VString("  <text action=\"append\">Tests passed: %d</text>", mTotalNumSuccesses));
        mLogger.rawLog(VString("  <text action=\"append\">Tests failed: %d</text>", mTotalNumErrors));

        if (mFailedTestSuiteNames.size() != 0)
            {
            VString names;
            for (VStringVector::const_iterator i = mFailedTestSuiteNames.begin(); i != mFailedTestSuiteNames.end(); ++i)
                {
                names += ' ';
                names += *i;
                }

            mLogger.rawLog(VString("  <text action=\"append\">These are the names of the failed tests:%s</text>", names.chars()));
            }

        mLogger.rawLog(VString(" </statusInfo>"));

        mLogger.rawLog(VString(" <statisticValue key=\"testCount\" value=\"%d\"/>", mTotalNumSuccesses + mTotalNumErrors));
        mLogger.rawLog(VString(" <statisticValue key=\"testsPassed\" value=\"%d\"/>", mTotalNumSuccesses));
        mLogger.rawLog(VString(" <statisticValue key=\"testsFailed\" value=\"%d\"/>", mTotalNumErrors));

        mLogger.rawLog(VString("</build>"));
        }
    catch (...) {} // prevent exceptions from escaping destructor
    }

