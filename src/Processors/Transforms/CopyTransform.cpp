#include <Processors/Transforms/CopyTransform.h>

namespace DB
{

CopyTransform::CopyTransform(const Block & header, size_t num_outputs)
    : IProcessor(InputPorts(1, header), OutputPorts(num_outputs, header))
{
}

IProcessor::Status CopyTransform::prepare()
{
    Status status = Status::Ready;

    while (status == Status::Ready)
    {
        status = !has_data ? prepareConsume()
                           : prepareGenerate();
    }

    return status;
}

IProcessor::Status CopyTransform::prepareConsume()
{
    auto & input = getInputPort();

    /// Check all outputs are finished or ready to get data.

    bool all_finished = true;
    for (auto & output : outputs)
    {
        if (output.isFinished())
            continue;

        all_finished = false;
    }

    if (all_finished)
    {
        input.close();
        return Status::Finished;
    }

    /// Try get chunk from input.

    if (input.isFinished())
    {
        for (auto & output : outputs)
            output.finish();

        return Status::Finished;
    }

    input.setNeeded();
    if (!input.hasData())
        return Status::NeedData;

    data = input.pullData();
    has_data = true;
    was_output_processed.assign(outputs.size(), false);

    return Status::Ready;
}

IProcessor::Status CopyTransform::prepareGenerate()
{
    bool all_outputs_processed = true;

    size_t chunk_number = 0;
    for (auto & output : outputs)
    {
        auto & was_processed = was_output_processed[chunk_number];
        ++chunk_number;

        if (was_processed)
            continue;

        if (output.isFinished())
            continue;

        if (!output.canPush())
        {
            all_outputs_processed = false;
            continue;
        }

        if (data.exception)
            output.pushException(data.exception);
        else
            output.push(data.chunk.clone());
        was_processed = true;
    }

    if (all_outputs_processed)
    {
        has_data = false;
        return Status::Ready;
    }

    return Status::PortFull;
}

}
